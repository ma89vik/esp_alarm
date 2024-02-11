set(srcs esp_alarm.c)

if(CONFIG_ESP_ALARM_DEBUG_MODE)
    list(APPEND srcs debug_helpers.c)
endif()

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "include"
                    REQUIRES "spiffs" "vfs" esp32-wifi-manager screens lvgl clock todo_list)
