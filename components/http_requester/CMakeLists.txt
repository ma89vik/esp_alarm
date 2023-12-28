idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    set(SRC "http_linux.c")
else()
    set(SRC "http_esp.c")
    set(REQ "esp_http_client" "mbedtls")
endif()

idf_component_register(SRCS ${SRC}
                       INCLUDE_DIRS "include"
                       REQUIRES ${REQ}
                       )


if(${target} STREQUAL "linux")
    find_package(CURL REQUIRED)
    target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/" ${CURL_INCLUDE_DIRS})
    target_link_libraries(${COMPONENT_LIB} PRIVATE ${CURL_LIBRARIES})
endif()