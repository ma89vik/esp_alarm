
idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    set(srcs "")
else()
    set(srcs "time_sync.c" )
endif()

list(APPEND srcs "alarm_list.c" "clock.c")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event
                    PRIV_REQUIRES esp_netif lwip)
