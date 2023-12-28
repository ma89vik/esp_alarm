idf_component_register(SRCS "test_clock.c"
                    INCLUDE_DIRS "."
                    REQUIRES clock esp_event nvs_flash protocol_examples_common console esp_netif)
