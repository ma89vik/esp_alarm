/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl.h"
#include "screens.h"
#include "weather.h"

#include "bsp/esp-bsp.h"

static const char *TAG = "DISP";

#define SHOW_BORDERS 1
#define TODO_ITEMS_MAX 10

LV_FONT_DECLARE(seven_segment)
LV_FONT_DECLARE(heiti_16)

ESP_EVENT_DEFINE_BASE(SCREENS_EVENT);

/*******************************************************************************
* Types definitions
*******************************************************************************/

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

typedef struct {
    weather_type_t type;
    const lv_img_dsc_t * img;
    const char * path;
} weather_img_map_entry;

static weather_img_map_entry icon_map[]= {
    {WEATHER_SUN, &sun, "sun.bin"},
    {WEATHER_FEW_CLOUDS, &partly_sun, "/spiffs/partly_sun.bin"},
    {WEATHER_SCATTERED_CLOUDS, &cloud, "/spiffs/cloud.bin"},
    {WEATHER_BROKEN_CLOUDS,  &cloud, "/spiffs/cloud.bin"},
    {WEATHER_SHOWER_RAIN, &rain , "/spiffs/rain.bin"},
    {WEATHER_RAIN, &partly_rain, "/spiffs/partly_rain.bin"},
    {WEATHER_THUNDERSTORM, &thunder, "/spiffs/thunder.bin"},
    {WEATHER_SNOW, &snow, "/spiffs/snow.bin"},
    {WEATHER_MIST, &cloud, "/spiffs/cloud.bin"},
    {WEATHER_UNKOWN, NULL, NULL},
};


/*******************************************************************************
* Function definitions
*******************************************************************************/
static void app_disp_lvgl_show_clock(lv_obj_t *screen, lv_group_t *group);
static void app_disp_lvgl_show_settings(lv_obj_t *screen, lv_group_t *group);
static void scroll_begin_event(lv_event_t *e);
static void tab_changed_event(lv_event_t *e);
static void set_tab_group(void);
static void desktop_create_weather_widget(lv_obj_t *parent);
static void desktop_create_todo_widget(lv_obj_t *parent);
static void desktop_create_set_alarm_widget(lv_obj_t *parent);


/*******************************************************************************
* Local variables
*******************************************************************************/

static lv_obj_t *tabview = NULL;
static lv_obj_t *tab_btns = NULL;
static lv_group_t *clock_group = NULL;
static lv_group_t *settings_group = NULL;

static lv_obj_t *time_label = NULL;
static char time_buf[20];

static lv_obj_t *weather_flex;
static lv_obj_t *weather_icon;
static lv_obj_t *temperature_label;

static lv_obj_t *todo_flex;
static lv_obj_t *todo_list[TODO_ITEMS_MAX];

// 0x47, 0xe0
static lv_color_t s_clock_font_color;

EventGroupHandle_t screenInitEventGroup;

/*******************************************************************************
* Public API functions
*******************************************************************************/

static void set_tile_style(lv_obj_t* obj)
{
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_bg_grad_color(obj, lv_color_make(0x05, 0x05, 0x05), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(obj, 255, 0);
}

static screen_cfg_t s_config;

void screens_show(screen_cfg_t * cfg)
{
    s_config = *cfg;

    bsp_display_lock(0);

    /* Tabview */
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 40);
    lv_obj_set_size(tabview, BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(tabview, &lv_font_montserrat_14, 0);
    /* Change animation time of moving between tabs */
    lv_obj_add_event_cb(lv_tabview_get_content(tabview), scroll_begin_event, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_add_event_cb(tabview, tab_changed_event, LV_EVENT_VALUE_CHANGED, NULL);

    /* Tabview buttons style */
    tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_color(tab_btns, lv_palette_lighten(LV_PALETTE_GREEN, 5), 0);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS | LV_STATE_CHECKED);

    /* Add tabs (the tabs are page (lv_page) and can be scrolled */
    lv_obj_t *tab_clock = lv_tabview_add_tab(tabview, LV_SYMBOL_HOME" Clock");
    lv_obj_t *tab_settings = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS" Settings");

    set_tile_style(tab_clock);

    /* Tileview */
    lv_obj_t *tileview;
    tileview = lv_tileview_create(tab_clock);
    lv_obj_center(tileview);
    lv_obj_t * clock_tile = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT | LV_DIR_LEFT);

    /* Input device group */
    lv_indev_t *indev = bsp_display_get_input_dev();

    app_disp_lvgl_show_clock(clock_tile, clock_group);
    app_disp_lvgl_show_settings(tab_settings, settings_group);

    lv_obj_t * todo_tile = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_RIGHT | LV_DIR_LEFT );
    desktop_create_todo_widget(todo_tile);

    bsp_display_unlock();
}

/*******************************************************************************
* Private API function
*******************************************************************************/



static void slider_brightness_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);

    assert(slider != NULL);

    /* Set brightness */
    bsp_display_brightness_set(lv_slider_get_value(slider));
}


static void app_disp_lvgl_show_clock(lv_obj_t *screen, lv_group_t *group)
{
    lv_obj_t *cont_row = NULL;

    /* Disable scrolling in this TAB */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Tile style */
    set_tile_style(screen);

    s_clock_font_color = lv_color_make(65, 255, 0);


    cont_row = lv_obj_create(screen);

    lv_obj_set_size(cont_row, BSP_LCD_H_RES, BSP_LCD_V_RES - 40);
    lv_obj_align(cont_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_bg_opa(cont_row, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(cont_row, LV_OPA_TRANSP, LV_STATE_DEFAULT);


    /* Label */
    static lv_style_t clock_style = {};
    lv_style_set_text_font(&clock_style, &seven_segment);  /*Set a larger font*/
    lv_style_set_text_color(&clock_style, s_clock_font_color);  /*Set a larger font*/

    desktop_create_weather_widget(cont_row);

    time_label = lv_label_create(cont_row);
    lv_obj_add_style(time_label, &clock_style, LV_STATE_DEFAULT);
    lv_label_set_text(time_label, "88:88");
    lv_group_add_obj(group, time_label);

}

void screens_set_time(uint8_t hours, uint8_t minutes)
{
    assert(hours < 25);
    assert(minutes < 61);

    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", hours, minutes);

    bsp_display_lock(0);
    lv_label_set_text(time_label, time_buf);
    bsp_display_unlock();
}

static void app_disp_lvgl_show_settings(lv_obj_t *screen, lv_group_t *group)
{
    lv_obj_t *cont_row;
    lv_obj_t *slider;

    /* Disable scrolling in this TAB */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* TAB style */
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_bg_color(screen, lv_color_make(0x00, 0x00, 0x00), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_make(0x05, 0x05, 0x05), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(screen, 255, 0);

    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    cont_row = lv_obj_create(screen);
    lv_obj_set_size(cont_row, BSP_LCD_H_RES - 20, 80);
    lv_obj_align(cont_row, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Label */
    lv_obj_t *lbl = lv_label_create(cont_row);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text_static(lbl, "Brightness: ");
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

    /* Slider */
    slider = lv_slider_create(cont_row);
    lv_obj_set_width(slider, BSP_LCD_H_RES - 180);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, APP_DISP_DEFAULT_BRIGHTNESS, false);
    lv_obj_center(slider);
    lv_obj_add_event_cb(slider, slider_brightness_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    if (group) {
        lv_group_add_obj(group, slider);
    }
}

static void set_tab_group(void)
{
    lv_indev_t *indev = bsp_display_get_input_dev();
    if (indev && clock_group && settings_group) {
        uint16_t tab = lv_tabview_get_tab_act(tabview);
        lv_group_set_editing(clock_group, false);
        lv_group_set_editing(settings_group, false);
        //lv_group_remove_obj(tab_btns);
        switch (tab) {
        case 0:
            lv_group_add_obj(clock_group, tab_btns);
            lv_indev_set_group(indev, clock_group);
            break;
        case 1:
            lv_group_add_obj(settings_group, tab_btns);
            lv_indev_set_group(indev, settings_group);
            break;
        }
        lv_tabview_set_act(tabview, tab, false);
    }
}

static void tab_changed_event(lv_event_t *e)
{
    /* Change scroll time animations. Triggered when a tab button is clicked */
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        set_tab_group();
    }
}

static void scroll_begin_event(lv_event_t *e)
{
    /* Change scroll time animations. Triggered when a tab button is clicked */
    if (lv_event_get_code(e) == LV_EVENT_SCROLL_BEGIN) {
        lv_anim_t *a = lv_event_get_param(e);
        if (a) {
            a->time = 300;
        }
    }
}

static void set_weather_image(weather_type_t weather_type)
{
    static

    bool img_found = false;
    for (int i = 0; i<sizeof(icon_map) / sizeof(weather_img_map_entry); i++) {
        if (weather_type == icon_map[i].type){
            lv_img_set_src(weather_icon, icon_map[i].img);

            img_found = true;
        }
    }

    if (!img_found) {
        ESP_LOGE(TAG, "Failed to find weather icon %d", weather_type);
    }
}

void screen_update_weather(float temperature_min, float temperature_max, weather_type_t weather_type)
{
    char weather_buf[15];
    bsp_display_lock(0);

    snprintf(weather_buf, sizeof(weather_buf), "%.1f~%.1f", temperature_min, temperature_max);

    set_weather_image(weather_type);
    lv_label_set_text(temperature_label, weather_buf);

    bsp_display_unlock();
}


static void desktop_create_weather_widget(lv_obj_t *parent)
{
    static lv_style_t weather_style = {};
    lv_style_set_text_color(&weather_style, s_clock_font_color);  /*Set a larger font*/

    weather_flex = lv_obj_create(parent);
    lv_obj_remove_style_all(weather_flex);
    lv_obj_set_style_pad_row(weather_flex, 10, 0);


    lv_obj_set_size(weather_flex, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_obj_center(weather_flex);
    lv_obj_set_flex_flow(weather_flex, LV_FLEX_FLOW_ROW);

    temperature_label =  lv_label_create(weather_flex);

    lv_label_set_text(temperature_label , "");
    lv_obj_add_style(temperature_label, &weather_style, LV_STATE_DEFAULT);
    lv_obj_set_flex_align(weather_flex, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    weather_icon = lv_img_create(weather_flex);
    lv_img_set_size_mode(weather_icon, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(weather_icon, 32);
}

void screen_todo_add_string(char *item)
{
    char str[100] = {};
    sprintf(str, "%s %s", "-", item);
    bsp_display_lock(0);

    for(int i = 0; i < TODO_ITEMS_MAX; i++) {
        if(todo_list[i] == NULL)  {
            todo_list[i] = lv_label_create(todo_flex);
            lv_label_set_text(todo_list[i], str);
            break;
        }
    }
    bsp_display_unlock();

}

static void desktop_create_todo_widget(lv_obj_t *parent)
{
    set_tile_style(parent);

    todo_flex = lv_obj_create(parent);
    lv_obj_remove_style_all(todo_flex);

    lv_obj_set_size(todo_flex, BSP_LCD_H_RES - 20, BSP_LCD_V_RES - 40);

    lv_obj_set_style_text_color(todo_flex, s_clock_font_color, 0);
    lv_obj_set_style_text_font(todo_flex, &heiti_16, 0);

    lv_obj_set_size(todo_flex, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_obj_center(todo_flex);
    lv_obj_set_flex_flow(todo_flex, LV_FLEX_FLOW_COLUMN);

    todo_list[0] = lv_label_create(todo_flex);
    lv_label_set_text(todo_list[0], "TODO:");

}

static lv_obj_t *alarm_time;
static uint8_t alarm_set_hours;
static uint8_t alarm_set_minutes;

typedef enum {
    DOWN_TEN_HOURS = 0,
    DOWN_ONE_HOUR,
    DOWN_TEN_MINUTES,
    DOWN_ONE_MINUTE,
    UP_TEN_HOURS,
    UP_ONE_HOUR,
    UP_TEN_MINUTES,
    UP_ONE_MINUTE,

} alarm_set_button_funcs_t;

static void set_alarm_adjust_time_cb(lv_event_t * e)
{

    static char char_buf[10];

    alarm_set_button_funcs_t button_event = (alarm_set_button_funcs_t)lv_event_get_user_data(e);

    switch(button_event) {
        case DOWN_TEN_HOURS:
            if (alarm_set_hours < 10) {
                alarm_set_hours = 20;
            } else {
                alarm_set_hours -= 10;
            }
            break;
        case DOWN_ONE_HOUR:
            if (alarm_set_hours == 0) {
                alarm_set_hours = 23;
            } else {
                alarm_set_hours--;
            }
            break;
        case UP_TEN_HOURS:
            if (alarm_set_hours >= 20) {
                alarm_set_hours = 0;
            } else {
                alarm_set_hours += 10;
            }
            break;
        case UP_ONE_HOUR:
            if (alarm_set_hours == 23) {
                alarm_set_hours = 0;
            } else {
                alarm_set_hours++;
            }
            break;
        case DOWN_TEN_MINUTES:
            if (alarm_set_minutes < 10) {
                alarm_set_minutes = 50;
            } else {
                alarm_set_minutes -= 10;
            }
            break;
        case DOWN_ONE_MINUTE:
            if (alarm_set_minutes == 0) {
                alarm_set_minutes = 59;
            } else {
                alarm_set_minutes--;
            }
            break;
        case UP_TEN_MINUTES:
            if (alarm_set_minutes >= 50) {
                alarm_set_minutes = 0;
            } else {
                alarm_set_minutes += 10;
            }
            break;
        case UP_ONE_MINUTE:
            if (alarm_set_minutes == 50) {
                alarm_set_minutes = 0;
            } else {
                alarm_set_minutes++;
            }
            break;;
    }
    snprintf(char_buf, sizeof(char_buf), "%02d:%02d", alarm_set_hours, alarm_set_minutes);
    lv_label_set_text(alarm_time, char_buf);
}

lv_obj_t *alarm_set_buttons[8];

static void set_alarm_create_buttons(lv_obj_t *parent, bool up)
{
    static lv_obj_t *alarm_inc_flex;
    static lv_obj_t *alarm_dec_flex;

    lv_obj_t * flex;

    if(up) {
        alarm_inc_flex = lv_obj_create(parent);
        flex = alarm_inc_flex;
    } else {
        alarm_dec_flex = lv_obj_create(parent);
        flex = alarm_dec_flex;
    }

    lv_obj_remove_style_all(flex);
    lv_obj_set_size(flex, BSP_LCD_H_RES, 40);
    lv_obj_align(flex, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(flex, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(flex, 2, 0);
    lv_obj_set_style_pad_bottom(flex, 2, 0);
    lv_obj_set_style_pad_column(flex, 43, 0);
    lv_obj_set_flex_align(flex, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for(int i = 0; i < 4; i++) {
        alarm_set_buttons[i + 4*up] = lv_btn_create(flex);

        if(up) {
            lv_obj_set_style_bg_img_src(alarm_set_buttons[i + 4*up], LV_SYMBOL_UP, 0);
        } else {
            lv_obj_set_style_bg_img_src(alarm_set_buttons[i + 4*up], LV_SYMBOL_DOWN, 0);
        }
        lv_obj_add_event_cb(alarm_set_buttons[i + 4*up], set_alarm_adjust_time_cb, LV_EVENT_CLICKED,  (void*)i + 4*up);
    }

}

static void alarm_set_save(lv_event_t * e)
{
    screens_alarm_set_event_data_t event_data = {
        .hours = alarm_set_hours,
        .minutes = alarm_set_minutes,
    };
    esp_event_post_to(s_config.event_loop, SCREENS_EVENT, SCREEN_ALARM_SET, &event_data, sizeof(event_data), portMAX_DELAY);
}

static void desktop_create_set_alarm_widget(lv_obj_t *parent)
{
    static lv_obj_t *alarm_flex;

    set_tile_style(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    /* Brightness */
    alarm_flex = lv_obj_create(parent);
    lv_obj_remove_style_all(alarm_flex);
    lv_obj_set_size(alarm_flex, BSP_LCD_H_RES, BSP_LCD_V_RES - 20);

    lv_obj_align(alarm_flex, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(alarm_flex, LV_FLEX_FLOW_COLUMN);
    //lv_obj_set_style_pad_top(alarm_flex, 2, 0);
    lv_obj_set_style_pad_bottom(alarm_flex, 2, 0);
    lv_obj_set_flex_align(alarm_flex, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Top buttons */
    set_alarm_create_buttons(alarm_flex, true);


    /* Label */
    static lv_style_t clock_style = {};
    lv_style_set_text_font(&clock_style, &seven_segment);  /*Set a larger font*/
    lv_style_set_text_color(&clock_style, s_clock_font_color);  /*Set a larger font*/

    alarm_time = lv_label_create(alarm_flex);
    lv_obj_add_style(alarm_time, &clock_style, LV_STATE_DEFAULT);
    lv_label_set_text(alarm_time, "00:00");

    /* Bottom buttons */
    //set_alarm_create_buttons(alarm_flex, false);

    /* Save button */
    static lv_obj_t *save_button;
    save_button = lv_btn_create(alarm_flex);
    static lv_obj_t *save_button_label;
    save_button_label = lv_label_create(save_button);
    lv_label_set_text(save_button_label, "Save");
    lv_obj_add_event_cb(save_button, alarm_set_save, LV_EVENT_CLICKED, NULL);
}
