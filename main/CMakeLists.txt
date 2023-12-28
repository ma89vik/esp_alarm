idf_component_register(SRCS esp_alarm.c
                    INCLUDE_DIRS "."
                    REQUIRES "spiffs" "vfs" esp32-wifi-manager screens lvgl clock todo_list)
