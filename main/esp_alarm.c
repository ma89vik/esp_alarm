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
#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_task_info.h"
#include "weather.h"
#include "openweathermap.h"
#include "todo_list.h"

#define WIFI_TASK_PRI 10

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
    ESP_LOGD(TAG, "screen event %ld\n", id);
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
    ESP_LOGD(TAG, "clock event %ld\n", id);
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

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
static TaskHandle_t wifi_task_handle;

void cb_connection_ok(void *pvParameter)
{
	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

    xTaskNotifyGive(wifi_task_handle);
}

static void wifi_task(void *pv)
{
    /* start the wifi manager */
	wifi_manager_start();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

    xTaskNotifyWait(0, 0, 0, portMAX_DELAY);

    ESP_LOGI(TAG, "Can start doing wifi stuff now\n");

    clock_time_sync();

    openweather_data_results_t weather_data = {};
    openweather_read("d6f4d6ccc640c139f1729d5ebd4746f2", "Shanghai", &weather_data);
    screen_update_weather(weather_data.temperature_min, weather_data.temperature_max, weather_data.type);

    todo_list_update("testkey");

    struct todo_item *item = NULL;
    while((item = todo_list_foreach(item))) {
        screen_todo_add_string(item->todo_str);
    }

    vTaskDelete(NULL);
}

#define MAX_TASK_NUM 20                         // Max number of per tasks info that it can store
#define MAX_BLOCK_NUM 20                        // Max number of per block info that it can store

static size_t s_prepopulated_num = 0;
static heap_task_totals_t s_totals_arr[MAX_TASK_NUM];
static heap_task_block_t s_block_arr[MAX_BLOCK_NUM];

static void esp_dump_per_task_heap_info(void)
{
    heap_task_info_params_t heap_info = {0};
    heap_info.caps[0] = MALLOC_CAP_8BIT;        // Gets heap with CAP_8BIT capabilities
    heap_info.mask[0] = MALLOC_CAP_8BIT;
    heap_info.caps[1] = MALLOC_CAP_32BIT;       // Gets heap info with CAP_32BIT capabilities
    heap_info.mask[1] = MALLOC_CAP_32BIT;
    heap_info.tasks = NULL;                     // Passing NULL captures heap info for all tasks
    heap_info.num_tasks = 0;
    heap_info.totals = s_totals_arr;            // Gets task wise allocation details
    heap_info.num_totals = &s_prepopulated_num;
    heap_info.max_totals = MAX_TASK_NUM;        // Maximum length of "s_totals_arr"
    heap_info.blocks = s_block_arr;             // Gets block wise allocation details. For each block, gets owner task, address and size
    heap_info.max_blocks = MAX_BLOCK_NUM;       // Maximum length of "s_block_arr"

    heap_caps_get_per_task_info(&heap_info);

    for (int i = 0 ; i < *heap_info.num_totals; i++) {
        printf("Task: %s -> CAP_8BIT: %d CAP_32BIT: %d\n",
                heap_info.totals[i].task ? pcTaskGetName(heap_info.totals[i].task) : "Pre-Scheduler allocs" ,
                heap_info.totals[i].size[0],    // Heap size with CAP_8BIT capabilities
                heap_info.totals[i].size[1]);   // Heap size with CAP32_BIT capabilities
    }

    printf("\n\n");
}

void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char *function_name)
{
    printf("%s was called but failed to allocate %d bytes with 0x%lX capabilities. \n", function_name, requested_size, caps);

    printf("Heap free size of this cap: 0x%X, largest cont. block 0x%X\n", heap_caps_get_free_size(caps), heap_caps_get_largest_free_block(caps));

    esp_dump_per_task_heap_info();
}

void app_main(void)
{
    heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);

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
