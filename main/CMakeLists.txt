idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_netif esp_event nvs_storage wifi_provisioning web ntp gps pps oled time_sync
)
