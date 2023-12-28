/**
 * Copyright (c) 2020 Marius
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

/**
 * Copyright (c) 2020 Marius
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "openweathermap.h"

#include "string.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "cJSON.h"
#include "weather.h"
#include "http_requester.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define KELVIN_TO_C 273.15
#define OPENWEATHER_READ_TIMEOUT_MS 10000
#define OWM_RESPONSE_MAX_LEN 4000

#define OWM_MAX_REQUEST_LEN 120
#define OWM_URL "api.openweathermap.org"
#define OWM_FORECAST_CNT 8

typedef struct {
    weather_type_t type;
    float temp;
    uint32_t time;
} openweather_data_t;

typedef struct {
    weather_type_t type;
    const char *icon_name;
} weather_img_map_entry;

static weather_img_map_entry s_icon_map[]= {
    {WEATHER_SUN, "01"},
    {WEATHER_FEW_CLOUDS, "02"},
    {WEATHER_SCATTERED_CLOUDS, "03"},
    {WEATHER_BROKEN_CLOUDS, "04"},
    {WEATHER_SHOWER_RAIN, "09"},
    {WEATHER_RAIN, "10"},
    {WEATHER_THUNDERSTORM, "11"},
    {WEATHER_SNOW, "13"},
    {WEATHER_MIST, "50"},
    {WEATHER_UNKOWN, ""},
};

static char *TAG = "openweathermap";

static esp_err_t openweather_parse_response(char *json_str, openweather_data_t *forecasts);
static esp_err_t openweather_calc_avg_weather(openweather_data_t *forecasts, openweather_data_results_t *avg_forecast);
static weather_type_t openweather_code_to_weather_type(char *code);

int openweather_read(char *api_key, char* city, openweather_data_results_t *weather_data)
{
    openweather_data_t forecasts[OWM_FORECAST_CNT] = {};

    esp_err_t ret = ESP_OK;
    char request[OWM_MAX_REQUEST_LEN];

    if (city == NULL) {
        ESP_LOGE(TAG, "No city argument");
    }


    snprintf(request, OWM_MAX_REQUEST_LEN, "/data/2.5/forecast?q=%s&cnt=%d&appid=%s", city, OWM_FORECAST_CNT, api_key );

    ESP_LOGI(TAG, "%s", request);
    char* response_data = malloc(OWM_RESPONSE_MAX_LEN*sizeof(char));
    ESP_GOTO_ON_FALSE(response_data, ESP_ERR_NO_MEM, err, TAG, "No mem for response data");

    ESP_GOTO_ON_ERROR(http_requester_get(OWM_URL, request, response_data, OWM_RESPONSE_MAX_LEN), err, TAG, "Failed to fetch OWM data");

    ESP_GOTO_ON_ERROR(openweather_parse_response(response_data, forecasts), err, TAG, "Failed to parse OWM data");

    ESP_GOTO_ON_ERROR(openweather_calc_avg_weather(forecasts, weather_data), err, TAG, "Failed to find average weather for today");

err:
    free(response_data);
    return ret;
}


static esp_err_t openweather_parse_response(char *json_str, openweather_data_t *forecasts) {

    int err;
    cJSON * root = cJSON_Parse(json_str);

    if(root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        err  = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }

    cJSON *forcast_list = cJSON_GetObjectItemCaseSensitive(root, "list");
    if (forcast_list==NULL) {
        ESP_LOGE(TAG, "Failed to get list fields");
        err  = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }

    cJSON *forecast_item = NULL;
    int i = 0;
    cJSON_ArrayForEach(forecast_item, forcast_list)
    {
        cJSON *main_data = cJSON_GetObjectItemCaseSensitive(forecast_item, "main");
        cJSON *weather = cJSON_GetObjectItemCaseSensitive(forecast_item, "weather");
        cJSON *time = cJSON_GetObjectItemCaseSensitive(forecast_item, "dt");

        if (!cJSON_IsNumber(time)) {
            ESP_LOGE(TAG, "Failed to get time field");
            err  = ESP_ERR_INVALID_ARG;
            goto cleanup;
        }

        forecasts[i].time = time->valueint;


        cJSON *temp_item = cJSON_GetObjectItemCaseSensitive(main_data, "temp");

        if (!cJSON_IsNumber(temp_item)) {
            ESP_LOGE(TAG, "Failed to get temp field");
            err  = ESP_ERR_INVALID_ARG;
            goto cleanup;
        }

        forecasts[i].temp = temp_item->valuedouble - KELVIN_TO_C;

        cJSON *subitem = cJSON_GetArrayItem(weather, 0);

        if (subitem==NULL) {
            ESP_LOGE(TAG, "Failed to get array index 0 from weather field");
            err  = ESP_ERR_INVALID_ARG;
            goto cleanup;
        }

        cJSON *icon = cJSON_GetObjectItemCaseSensitive(subitem, "icon");

        if (!cJSON_IsString(icon)) {
            ESP_LOGE(TAG, "Failed to get icon field");
            err  = ESP_ERR_INVALID_ARG;
            goto cleanup;
        }

        forecasts[i].type = openweather_code_to_weather_type(icon->valuestring);

        i++;
    }

    err =ESP_OK;


cleanup:

    cJSON_Delete(root);
    return err;
}


static weather_type_t openweather_code_to_weather_type(char *code) {
    for (int i = 0; i<sizeof(s_icon_map)/sizeof(weather_img_map_entry); i++) {
        if (!strncmp(code, s_icon_map[i].icon_name, 2)) {
            return s_icon_map[i].type;
        }
    }

    return WEATHER_UNKOWN;
}

static esp_err_t openweather_calc_avg_weather(openweather_data_t *forecasts, openweather_data_results_t *avg_forecast)
{
    esp_err_t err = ESP_OK;
    float temperature_min = 100;
    float temperature_max = -100;

    weather_type_t weather_type = forecasts[0].type;
    time_t time = forecasts[0].time;

    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    struct tm tm_struct;
    localtime_r(&time, &tm_struct);
    int today = tm_struct.tm_mday;

    for (int i = 0; i < OWM_FORECAST_CNT; i++) {

        if (forecasts[i].temp > temperature_max) {
            temperature_max = forecasts[i].temp;
        }
        if (forecasts[i].temp < temperature_min) {
            temperature_min = forecasts[i].temp;
        }
        time = forecasts[i].time;
        localtime_r(&time, &tm_struct);

        ESP_LOGD(TAG, "Weather data %dth, %d hours: type %d, temp %f \n", tm_struct.tm_mday, tm_struct.tm_hour, forecasts[i].type, forecasts[i].temp);

        if (tm_struct.tm_mday != today) {
            /*Only consider todays weather*/
            break;
        }

        if ( (tm_struct.tm_hour >= 12) &&  (tm_struct.tm_hour <= 16)) {
            /*Midday weather best represents todays weather*/
            weather_type = forecasts[i].type;
        }

    }

    avg_forecast->temperature_min = temperature_min;
    avg_forecast->temperature_max = temperature_max;
    avg_forecast->type = weather_type;


    return err;
}

