/**
 * Copyright (c) 2020 Marius
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "weather.h"

#include "time.h"
#include "aqi.h"
#include "openweathermap.h"
#include "wifi_manager.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include <string.h>

#define LOC_STR_MAX_LEN 30
#define NVS_WEATHER_KEY "weather_data"
#define WEATHER_UPDATE_PERIOD_S 3600

static char *TAG = "weather";


typedef struct {
    uint16_t pm25;
    float temp;
    weather_type_t weather_type;
    time_t timestamp;
    char loc[LOC_STR_MAX_LEN];
} weather_data_t;

typedef struct weather_t {
    weather_cfg_t cfg;
    weather_data_t weather_data;
}weather_t;

static esp_err_t weather_fetch_from_nvs(weather_data_t *weather_data);
static bool weather_is_loc_correct(weather_data_t * weather_data, weather_t *weather);
static bool weather_is_outdated(weather_data_t * weather_data);
static esp_err_t weather_store_to_nvs(weather_data_t *weather_data);


static void weather_task(void *vParameters) {
     /* 1. Readvalue from NVS
    2. if outdated get from web
    3. update through cbs
    */

    weather_t *weather = (weather_t*)vParameters;

    weather_data_t *weather_data = &(weather->weather_data);
    weather_fetch_from_nvs(weather_data);

    ESP_LOGD(TAG, "Fetched weather data from NVS: pm25 = %d, loc = %s, timestamp = %ld",
            weather_data->pm25, weather_data->loc, weather_data->timestamp);

    if (!weather_is_outdated(weather_data) & weather_is_loc_correct(weather_data, weather)) {

        goto no_update;
    }

    if (wifi_manager_request_access() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init wifi");
        wifi_manager_finished();
        goto exit;
    }
    /* Weather data is outdate, fetch new */
    ESP_LOGI(TAG, "Fetch online weather data");
    weather_data->pm25 = aqi_read(weather->cfg.loc);
    weather_data->timestamp = time(NULL);
    strncpy( weather_data->loc, weather->cfg.loc, LOC_STR_MAX_LEN);

    openweather_data_t data;
    int openweather_ret = openweather_read(weather->cfg.loc, &data);
    weather_data->temp = data.temp;
    weather_data->weather_type = data.type;

    /* Store new weather data in NVS if sucessful read */
    if (weather_data->pm25 != 0 && (openweather_ret == 0)) {
        ESP_LOGI(TAG, "Successfully fetched weather data online");
        weather_store_to_nvs(weather_data);
    }

    wifi_manager_finished();

no_update:
    ESP_LOGI(TAG, "Weather and AQI data is up to date");

    if (weather->cfg.pm25_cb != NULL) {
        weather->cfg.pm25_cb(weather_data->pm25);
    }

    if (weather->cfg.weather_cb != NULL) {
        weather->cfg.weather_cb(weather_data->temp, weather_data->weather_type);
    }
exit:
    vTaskDelete(NULL);
}



/* Async call for reading PM 2.5 from aqicn.org
   returns the value cached in NVS. If new value is needed then
   this will be fetched online and updated through the callback */
void weather_update(weather_handle_t weather){

    xTaskCreate(weather_task, "weather update", 4096, (void*)weather, 7, NULL);

}


static esp_err_t weather_fetch_from_nvs(weather_data_t *weather_data) {
    nvs_handle_t nvs_handle;
    int err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
       ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }

    // Read
    size_t len = sizeof(weather_data_t);
    err = nvs_get_blob(nvs_handle, NVS_WEATHER_KEY, weather_data, &len);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "The NVS value is not initialized yet!\n");
        memset(weather_data, 0, sizeof(weather_data_t));
    }
    return err;
}

static esp_err_t weather_store_to_nvs(weather_data_t *weather_data) {
    nvs_handle_t nvs_handle;
    int err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
       ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }

    // Read
    size_t len = sizeof(weather_data_t);
    err = nvs_set_blob(nvs_handle, NVS_WEATHER_KEY, weather_data, len);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to NVS, err: %s!", esp_err_to_name(err));
    }

    err = nvs_commit(nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit  to NVS, err: %s!", esp_err_to_name(err));
    }

    return err;
}

static bool weather_is_outdated(weather_data_t * weather_data) {
    return ((time(NULL) - weather_data->timestamp) > WEATHER_UPDATE_PERIOD_S);
}

static bool weather_is_loc_correct(weather_data_t * weather_data, weather_t *weather) {
    return !strncmp(weather->cfg.loc, weather_data->loc, LOC_STR_MAX_LEN);
}

/* Init the module with location for readings*/
weather_handle_t weather_init(weather_cfg_t *cfg) {
    weather_t *weather = malloc(sizeof(weather_t));

    if(!weather) {
        ESP_LOGE(TAG, "Failed to malloc weather");
        return NULL;
    }


    weather->cfg.loc = strndup(cfg->loc, LOC_STR_MAX_LEN);
    weather->cfg.pm25_cb = cfg->pm25_cb;
    weather->cfg.weather_cb = cfg->weather_cb;

    return weather;
}