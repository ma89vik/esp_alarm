#pragma once

#include <stdbool.h>

#include "lvgl.h"
#include "esp_err.h"

#define BSP_LCD_H_RES              (320)
#define BSP_LCD_V_RES              (240)

lv_disp_t *bsp_display_start(void);

bool bsp_display_lock(uint32_t timeout);

void bsp_display_unlock();

lv_indev_t *bsp_display_get_input_dev();

lv_indev_t *bsp_display_indev_init();

void bsp_display_brightness_set(uint32_t brightness);

esp_err_t bsp_i2c_init(void);
