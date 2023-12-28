// Copyright (c) 2020 Marius
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "weather.h"
#include "esp_err.h"

typedef struct {
    weather_type_t type;
    float temperature_min;
    float temperature_max;
} openweather_data_results_t;

int openweather_read(char *api_key, char* city, openweather_data_results_t *weather_data);
