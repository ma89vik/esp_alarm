idf_build_get_property(target IDF_TARGET)
if(NOT ${target} STREQUAL "linux")
    return()
endif()

idf_component_register(SRCS "esp_linux_bsp.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES lvgl
                     )

find_package(SDL2 REQUIRED)

add_library(lvgl ALIAS __idf_lvgl__lvgl)

include(FetchContent)

FetchContent_Declare(lv_drivers
                     SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lv_drivers)

FetchContent_MakeAvailable(lv_drivers)

target_include_directories(lv_drivers PUBLIC "${CMAKE_CURRENT_LIST_DIR}/" ${SDL2_INCLUDE_DIRS})
target_link_libraries(lv_drivers PRIVATE ${SDL2_LIBRARIES})
target_compile_options(lv_drivers PRIVATE "-Wno-unused-variable")

# Update component with the external project
target_link_libraries(${COMPONENT_LIB} PUBLIC lv_drivers)

target_include_directories(${COMPONENT_LIB} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

