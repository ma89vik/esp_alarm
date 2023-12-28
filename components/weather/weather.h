// Copyright (c) 2020 Marius
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "stdint.h"
#include "time.h"

typedef enum {
    WEATHER_SUN,
    WEATHER_FEW_CLOUDS,
    WEATHER_SCATTERED_CLOUDS,
    WEATHER_BROKEN_CLOUDS,
    WEATHER_SHOWER_RAIN,
    WEATHER_RAIN,
    WEATHER_THUNDERSTORM,
    WEATHER_SNOW,
    WEATHER_MIST,
    WEATHER_UNKOWN,
} weather_type_t;

typedef void (*update_aqi_cb)(uint16_t pm25);
typedef void (*update_weather_cb)(float temp, weather_type_t weather_type);

typedef struct {
    char *loc;
    update_aqi_cb pm25_cb;
    update_weather_cb weather_cb;
} weather_cfg_t;

typedef struct weather_t* weather_handle_t;

/* Async call for reading PM 2.5 from aqicn.org
   returns the value cached in NVS. If new value is needed then
   this will be fetched online and updated through the callback */
void weather_update(weather_handle_t weather);


weather_handle_t weather_init(weather_cfg_t *cfg);
