idf_component_register(SRCS "openweathermap.c"
                       INCLUDE_DIRS "."
                       REQUIRES log http_requester json)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
