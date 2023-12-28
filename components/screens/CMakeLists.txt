idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    set(target_priv_reqs "esp_linux_bsp")
else()
    set(target_priv_reqs "esp-box-3")
endif()

idf_component_register(SRCS "screens.c"
                            "seven_segment_125.c"
                            "seven_segment_small.c"
                            "res/cloud.c"
                            "res/sun.c"
                            "res/moon.c"
                            "res/partly_moon.c"
                            "res/partly_rain.c"
                            "res/partly_sun.c"
                            "res/rain.c"
                            "res/snow.c"
                            "res/thunder.c"
                            "res/wind.c"
                            "res/heiti_16.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES lvgl ${target_priv_reqs}
                    REQUIRES weather esp_event)
