#pragma once

#include <time.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

typedef enum {
    CLOCK_ALARM_REPEAT_NONE,
    CLOCK_ALARM_REPEAT_DAILY,
    CLOCK_ALARM_REPEAT_WEEKLY,
} clock_alarm_repeat_t;

typedef struct {
    struct tm alarm_time;
    clock_alarm_repeat_t repeat;
    int id;
} clock_alarm_t;

ESP_EVENT_DECLARE_BASE(CLOCK_EVENT);

typedef enum {
    CLOCK_ALARM_TRIGGERED, /*!<  an alarm has triggered */
    CLOCK_TIME_CHANGED,    /*!<  Time has changed */
} clock_event_id_t;

typedef struct {
    union {
        clock_alarm_t alarm_triggered;
        struct {
            uint8_t hours;
            uint8_t minutes;
        } time_changed;
    };
} clock_event_data_t;

typedef struct {
    esp_event_loop_handle_t event_loop;
} clock_cfg_t;

esp_err_t clock_init(clock_cfg_t *cfg);

void clock_add_alarm(struct tm *time, clock_alarm_repeat_t repeat);

uint32_t clock_alarm_get_num(void);

/* Returns copys of the alarms, can safely be free-ed/manipulated */
void clock_get_alarms(clock_alarm_t *alarms, size_t num_alarms);

void clock_delete_all_alarm(void);

bool clock_get_active_alarm(clock_alarm_t *active_alarm);

esp_err_t clock_time_sync(void);
