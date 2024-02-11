#include <stdio.h>
#include "bsp/esp-bsp.h"
#include "screens.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "lvgl.h"


/* Weather icons */
LV_IMG_DECLARE(cloud);
LV_IMG_DECLARE(sun);
LV_IMG_DECLARE(moon);
LV_IMG_DECLARE(partly_moon);
LV_IMG_DECLARE(partly_rain);
LV_IMG_DECLARE(partly_sun);
LV_IMG_DECLARE(rain);
LV_IMG_DECLARE(snow);
LV_IMG_DECLARE(thunder);
LV_IMG_DECLARE(wind);

void app_main(void)
{
    printf("Start display\n");
    bsp_display_start();

    lv_obj_t *flex = lv_obj_create(lv_scr_act());
    lv_obj_set_flex_flow(flex, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_t *img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &cloud);

    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &sun);

    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &moon);

    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &partly_moon);

    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &partly_rain);


    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &partly_sun);


    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &rain);


    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &snow);


    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &thunder);

    img = lv_img_create(flex);
    lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(img, 32);
    lv_img_set_src(img, &wind);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
