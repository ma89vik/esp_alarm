#include <stdio.h>
#include "esp_linux_bsp.h"
#include "screens.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"

static void test_set_weather()
{
    screen_update_weather(23.2, 25.4, WEATHER_BROKEN_CLOUDS);
}

static void test_alarm_cb(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    screens_alarm_set_event_data_t *data = (screens_alarm_set_event_data_t*)event_data;
    printf("set_alarm %d:%d\n", data->hours, data->minutes);
}

void app_main(void)
{
    printf("Start display\n");
    bsp_display_start();

    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "Test screen event",
        .task_priority = 5,
        .task_stack_size = 4000,
        .task_core_id = 0
    };

    static esp_event_loop_handle_t loop_handle;

    esp_event_loop_create(&loop_args, &loop_handle);

    esp_event_handler_register_with(loop_handle, SCREENS_EVENT, SCREEN_ALARM_SET, test_alarm_cb, NULL);


    screen_cfg_t cfg = {
        .event_loop = loop_handle,
    };

    screens_show(&cfg);
    int i = 0;

    test_set_weather();

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        screens_set_time(2, i  % 60);
        i++;
    }
}
