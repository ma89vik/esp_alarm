#include <stdio.h>
#include "esp_linux_bsp.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "lv_drv_conf.h"
#include "lv_drivers/sdl/sdl.h"

#define MAX_DELAY 500

static const char TAG[] = "linux-bsp";


bool bsp_display_lock(uint32_t timeout)
{
    return true;
}

void bsp_display_unlock()
{

}

void bsp_display_brightness_set(uint32_t brightness)
{

}

static lv_color_t frame_buf[BSP_LCD_H_RES * BSP_LCD_V_RES];
static lv_disp_draw_buf_t draw_buf_dsc;
static lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
static lv_indev_drv_t indev_drv;
static lv_indev_t* indev;

lv_indev_t *bsp_display_get_input_dev()
{
    return indev;
}

lv_indev_t *bsp_display_indev_init()
{
    lv_indev_drv_init(&indev_drv);      /*Basic initialization*/

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;

    /*Register the driver in LVGL and save the created input device object*/
    indev = lv_indev_drv_register(&indev_drv);

    return indev;
}


#define LVGL_TICK_PERIOD_MS 100

static void lvgl_tick(void *pv)
{
    /* Tell LVGL how many milliseconds has elapsed */
    while(1) {
        lv_tick_inc(LVGL_TICK_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS));
    }

}

static void lvgl_port_task(void *arg)
{
    uint32_t task_delay_ms = MAX_DELAY;

    ESP_LOGI(TAG, "Starting LVGL task");

    while(1) {
        if (bsp_display_lock(0)) {
            task_delay_ms = lv_timer_handler();
            bsp_display_unlock();
        }
        if ((task_delay_ms > MAX_DELAY) || (1 == task_delay_ms)) {
            task_delay_ms = MAX_DELAY;
        } else if (task_delay_ms < 1) {
            task_delay_ms = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }

    /* Close task */
    vTaskDelete( NULL );
}

static void bsp_display_lcd_init()
{
    /*Init display hardware*/
    sdl_init();

    lv_disp_draw_buf_init(&draw_buf_dsc, frame_buf, NULL, BSP_LCD_H_RES * BSP_LCD_V_RES); /*Initialize the display buffer*/

    lv_disp_drv_init(&disp_drv);

    /*Set the resolution of the display*/
    disp_drv.hor_res = BSP_LCD_H_RES;
    disp_drv.ver_res = BSP_LCD_V_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = sdl_display_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc;

    disp_drv.direct_mode = true;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);

}

lv_disp_t *bsp_display_start(void)
{
    /* LVGL init */
    lv_init();

    xTaskCreate(lvgl_port_task, "LVGL task", 10000, NULL, 10, NULL);

    bsp_display_lcd_init();

    bsp_display_indev_init();

    xTaskCreate(lvgl_tick, "LVGL tick task", 3000, NULL, 20, NULL);

    return NULL;
}