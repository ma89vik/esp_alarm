/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"
#include "esp_event.h"
#include "bsp/esp-bsp.h"
#include "screens.h"
#include "clock.h"
#if !CONFIG_IDF_TARGET_LINUX
#include "wifi_manager.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_task_info.h"
#include "weather.h"
#include "openweathermap.h"
#include "todo_list.h"
#include "debug_helpers.h"

#ifdef CONFIG_HEAP_TRACING
    #include <esp_heap_trace.h>
#endif //CONFIG_HEAP_TRACING

#define WIFI_TASK_PRI 10

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static const char *TAG = "esp_alarm";

static esp_event_loop_handle_t s_event_loop;

static void screen_event_handler(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    switch (id)
    {
    case SCREEN_ALARM_SET:
        screens_alarm_set_event_data_t *data = (screens_alarm_set_event_data_t*)event_data;

        struct tm alarm_tm = {
            .tm_hour = data->hours,
            .tm_min = data->minutes,
        };

        clock_add_alarm(&alarm_tm, CLOCK_ALARM_REPEAT_DAILY);
        break;

    default:
        break;
    }
    ESP_LOGD(TAG, "screen event %"PRIu32"\n", id);
}

static void clock_event_handler(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    clock_event_data_t *data = (clock_event_data_t*)event_data;
    switch (id)
    {
    case CLOCK_ALARM_TRIGGERED:
        // Play sound and animation
        break;
    case CLOCK_TIME_CHANGED:
        screens_set_time(data->time_changed.hours, data->time_changed.minutes);
    default:
        break;
    }
    ESP_LOGD(TAG, "clock event %"PRIi32"\n", id);
}


void app_init_event_handler(void)
{
    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "esp_alarm_events",
        .task_priority = 10,
        .task_stack_size = 4000,
        .task_core_id = 0
    };

    esp_event_loop_create(&loop_args, &s_event_loop);

    /* Register all events */
    esp_event_handler_register_with(s_event_loop, SCREENS_EVENT, ESP_EVENT_ANY_ID, screen_event_handler, NULL);
    esp_event_handler_register_with(s_event_loop, CLOCK_EVENT, ESP_EVENT_ANY_ID, clock_event_handler, NULL);
}

static TaskHandle_t wifi_task_handle;

#if !CONFIG_IDF_TARGET_LINUX
void cb_connection_ok(void *pvParameter)
{
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

    xTaskNotifyGive(wifi_task_handle);
}
#endif

static void wifi_task(void *pv)
{
    /* start the wifi manager */
#if !CONFIG_IDF_TARGET_LINUX
	wifi_manager_start();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

    xTaskNotifyWait(0, 0, 0, portMAX_DELAY);

    ESP_LOGI(TAG, "Can start doing wifi stuff now\n");

    clock_time_sync();
#endif

#if CONFIG_IDF_TARGET_LINUX
    // Hack on linux to make sure screen is ready for writing
    vTaskDelay(pdMS_TO_TICKS(3000));
#endif

    const uint32_t UPDATE_INTERVAL_WEATHER_MS = 60*60*1000; // 1 hour
    const uint32_t UPDATE_INTERVAL_TODO_MS    = 60*1000;   // 1 minute

    const uint32_t MIN_UPDATE_INTERVAL = MIN(UPDATE_INTERVAL_WEATHER_MS, UPDATE_INTERVAL_TODO_MS);
    const uint32_t MAX_UPDATE_INTERVAL = MAX(UPDATE_INTERVAL_WEATHER_MS, UPDATE_INTERVAL_TODO_MS);

    /* Check that all update intervals are a multiple of min update interval */
    _Static_assert(UPDATE_INTERVAL_WEATHER_MS % MIN_UPDATE_INTERVAL == 0, "Weather update interval must be a multiple of min update interval");
    _Static_assert(UPDATE_INTERVAL_TODO_MS % MIN_UPDATE_INTERVAL == 0, "Weather update interval must be a multiple of min update interval");

    uint32_t update_interval_counter = 0;
    while(1) {
        if (update_interval_counter % UPDATE_INTERVAL_WEATHER_MS == 0) {
            /* Update weather data */
            openweather_data_results_t weather_data = {};
            openweather_read("d6f4d6ccc640c139f1729d5ebd4746f2", "Shanghai", &weather_data);
            screen_update_weather(weather_data.temperature_min, weather_data.temperature_max, weather_data.type);
        }

        if(update_interval_counter % UPDATE_INTERVAL_TODO_MS == 0) {
            /* Update todo list */
            todo_list_update("testkey");

            struct todo_item *item = NULL;
            while((item = todo_list_foreach(item))) {
                screen_todo_add_string(item->todo_str);

            }
        }

        /* Refresh information */
        vTaskDelay(pdMS_TO_TICKS(MIN_UPDATE_INTERVAL));
        update_interval_counter += MIN_UPDATE_INTERVAL;

        /* Wrap around when at MAX_UPDATE_INTERVAL */
        if (update_interval_counter >= MAX_UPDATE_INTERVAL) {
            update_interval_counter = 0;
        }
    }


    vTaskDelete(NULL);
}


void app_main(void)
{
    /* Init debug helpers if in debug mode */
#if CONFIG_ESP_ALARM_DEBUG_MODE
    debug_helpers_init();
#endif

    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();

    /* Initialize display and LVGL */
    bsp_display_start();

    xTaskCreatePinnedToCore(wifi_task, "Wifi_task", 10000, NULL, WIFI_TASK_PRI, &wifi_task_handle, 0);

    app_init_event_handler();

    clock_cfg_t clock_cfg = {
        .event_loop = s_event_loop,
    };
    clock_init(&clock_cfg);

    /* Set default display brightness */
    bsp_display_brightness_set(APP_DISP_DEFAULT_BRIGHTNESS);

    /* Add and show LVGL objects on display */
    screen_cfg_t screen_cfg = {
        .event_loop = s_event_loop,
    };

    screens_show(&screen_cfg);

    /* Initialize SPI flash file system and show list of files on display */
    //app_disp_fs_init();

    ESP_LOGI(TAG, "Example initialization done.");

}
