#include <stdio.h>
#include "esp_err.h"
#include "clock.h"

#include "unity.h"
#include "unity_test_utils.h"


static volatile bool alarm_triggered = false;

static void test_alarm_cb(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    clock_event_data_t *data = (clock_event_data_t*)event_data;
    printf("alarm triggered %d:%d\n", data->alarm_triggered.alarm_time.tm_hour, data->alarm_triggered.alarm_time.tm_min);
    alarm_triggered = true;
}


TEST_CASE("Test clock", "[esp-clock]")
{
    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "Test clock event",
        .task_priority = 5,
        .task_stack_size = 4000,
        .task_core_id = 0
    };

    static esp_event_loop_handle_t loop_handle;

    esp_event_loop_create(&loop_args, &loop_handle);

    esp_event_handler_register_with(loop_handle, CLOCK_EVENT, CLOCK_ALARM_TRIGGERED, test_alarm_cb, NULL);

    clock_cfg_t cfg = {
        .event_loop = loop_handle,
    };

    clock_init(&cfg);

    time_t rawtime;

    clock_alarm_t active_alarm = {0};
    TEST_ASSERT_TRUE(!clock_get_active_alarm(&active_alarm));

    time ( &rawtime );
    struct tm alarm_time = {};
    localtime_r(&rawtime, &alarm_time);
    alarm_time.tm_min++;

    clock_add_alarm(&alarm_time, CLOCK_ALARM_REPEAT_DAILY);
    while(!clock_get_active_alarm(&active_alarm)) {
        vTaskDelay(pdMS_TO_TICKS(500));
        printf("Checking alarms...\n");
        if(alarm_triggered) {
            break;
        }
    }
}

void app_main(void)
{
    unity_run_menu();
}
