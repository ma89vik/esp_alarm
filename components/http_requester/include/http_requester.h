#pragma once

#include "esp_err.h"

esp_err_t http_requester_get(char* host, char* path, char* response_data, size_t len);
