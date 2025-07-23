# Test application CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Define the project
project(SQLiteTest_Tests)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/3rdparty.cmake)

# Set up the test project
set(EXTRA_COMPONENT_DIRS "../main")

# Register the test project for ESP32-C3 target
idf_component_register(
    SRCS 
        "unit/test_main.c"
        "unit/network/test_wifi_manager.c"
        "unit/network/test_http_client.c"
        "unit/sensors/test_sensor_health.c"
        "unit/core/test_config_manager.c"
        "unit/core/test_health_monitor.c"
        "unit/mocks/mockI2cFunctions.c"
        "unit/mocks/mockWifiFunctions.c"
        "unit/test_result_tracker.c"
        # Add more test files here as they are created
    INCLUDE_DIRS 
        "unit"
        "unit/mocks"
        "../main"
        "../main/sensors"
        "../main/core"
        "../main/network"
    REQUIRES 
        unity
        esp_common
        esp_system
        driver
        esp_wifi
        esp_event
        nvs_flash
        spiffs
        main
)

# Set up Unity configuration for ESP-IDF
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    -DUNITY_INCLUDE_CONFIG_H
    -DCMOCK_MEM_DYNAMIC
    -DTEST_MODE
)

# Add compiler flags for testing
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-function
    -Wno-unused-variable
)

# Enable Unity testing features
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    -DUNITY_ENABLE_FLOAT
    -DUNITY_ENABLE_DOUBLE
    -DUNITY_ENABLE_64BIT
    -DUNITY_ENABLE_COLOR
)
