#include <stdio.h>
#include "esp_linux_bsp.h"
#include "screens.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "openweathermap.h"
#include "todo_list.h"

static void test_set_weather()
{
    openweather_data_results_t weather_data = {};
    openweather_read("d6f4d6ccc640c139f1729d5ebd4746f2", "Shanghai", &weather_data);
    screen_update_weather(weather_data.temperature_min, weather_data.temperature_max, weather_data.type);

}

static void test_set_todo()
{
    todo_list_update("testkey");

    struct todo_item *item = NULL;
    while((item = todo_list_foreach(item))) {
        screen_todo_add_string(item->todo_str);
    }
}


void app_main(void)
{
    printf("Start display\n");
    bsp_display_start();

    screens_show();
    int i = 0;

    test_set_weather();
    test_set_todo();

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        screens_set_time(2, i  % 60);
        i++;
    }
}
