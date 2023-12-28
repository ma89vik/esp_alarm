

#include "clock.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "esp_err.h"
#include "esp_log.h"

static const char TAG[] = "clock";

ESP_EVENT_DEFINE_BASE(CLOCK_EVENT);

static clock_cfg_t s_config;


static void second_tick_cb( TimerHandle_t xTimer)
{
    static struct tm tm_prev;

    time_t now;
    struct tm now_tm = {};
    time(&now);
    localtime_r(&now, &now_tm);

    /* only perform checks/updates if minutes have changed */
    if(tm_prev.tm_min == now_tm.tm_min) {
        return;
    } else {
        tm_prev = now_tm;
    }

    clock_event_data_t data = {
        .time_changed.hours = now_tm.tm_hour,
        .time_changed.minutes = now_tm.tm_min,
    };
    esp_event_post_to(s_config.event_loop, CLOCK_EVENT, CLOCK_TIME_CHANGED, &data, sizeof(clock_event_data_t), portMAX_DELAY);

    clock_alarm_t alarm = {};
    if(clock_get_active_alarm(&alarm)) {
        esp_event_post_to(s_config.event_loop, CLOCK_EVENT, CLOCK_ALARM_TRIGGERED, &alarm, sizeof(clock_event_data_t), portMAX_DELAY);
    }
}

static TimerHandle_t clock_sec_timer;

esp_err_t clock_init(clock_cfg_t *cfg)
{
    esp_err_t err = ESP_OK;
    s_config = *cfg;

    clock_sec_timer = xTimerCreate("clock_sec_timer", pdMS_TO_TICKS(1000), pdTRUE, 0, second_tick_cb);
    if(!clock_sec_timer) {
        ESP_LOGE(TAG, "Failed to create clock timer");
        err = ESP_ERR_NO_MEM;
        goto err;
    }

    if(!xTimerStart(clock_sec_timer, 0)) {
        ESP_LOGE(TAG, "Failed to start clock timer\n");
        err = ESP_ERR_INVALID_STATE;
        goto err;
    }

err:
    return err;
}
