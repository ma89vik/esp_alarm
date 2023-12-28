/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once


/* Default screen brightness */
#define APP_DISP_DEFAULT_BRIGHTNESS  (50)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "esp_event.h"

#include "weather.h"

ESP_EVENT_DECLARE_BASE(SCREENS_EVENT);

typedef enum {
    SCREEN_ALARM_SET, /*!< A new alarm has been set */
} screens_event_id_t;

typedef struct {
    uint8_t hours;
    uint8_t minutes;
} screens_alarm_set_event_data_t;

typedef struct {
    esp_event_loop_handle_t event_loop;
} screen_cfg_t;

/**
 * @brief Add and show LVGL objects on display
 */
void screens_show(screen_cfg_t * cfg);

void screens_set_time(uint8_t hours, uint8_t minutes);

void screen_update_weather(float temperature_min, float temperature_max, weather_type_t weather_type);

void screen_todo_add_string(char *item);


#ifdef __cplusplus
}
#endif
