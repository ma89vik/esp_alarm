idf_component_register(SRCS "test_screen_integration.c"
                    INCLUDE_DIRS "."
                    REQUIRES screens esp_linux_bsp weather http_requester todo_list)
