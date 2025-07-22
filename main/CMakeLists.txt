# CMakeLists.txt for SQLiteTest main component

idf_component_register(
    SRCS
        "main.cpp"
        "core/rtos_manager.cpp"
        "core/I2CManager.cpp"
        "core/config_manager.cpp"
        "core/json_config_provider.cpp"
        "core/sensor_registry.cpp"
        "sensors/AHT21Sensor.cpp"
        "sensors/ENS160Sensor.cpp"
        "network/wifi_manager.c"
        "network/sntp_manager.c"
        "network/http_client.c"
        "network/http_server.c"
        "network/secure_credentials.c"
        "core/health_monitor.cpp"
        "network/health_server.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi
             esp_http_client
             esp_timer
             json
             nvs_flash
             esp_driver_gpio
             driver
             esp_pm
             spiffs
)

# Set C++ standard
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++17)

# Enable C++ exceptions (optional, but useful for our error handling)
target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions)

# Embed files in SPIFFS
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)

