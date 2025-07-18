# Unit Test CMakeLists.txt for ESP32-C3 Environmental Monitor
cmake_minimum_required(VERSION 3.16.0)

# Register test component
idf_component_register(
    SRCS 
        "unit/test_main.c"
        "unit/network/test_wifi_manager.c"
        "unit/network/test_http_client.c"
        "unit/sensors/test_sensor_health.c"
        # Add more test files here as they are created
    
    INCLUDE_DIRS 
        "unit"
        "unit/mocks"
        "../main"
        "../main/network"
        "../main/sensors"
    
    REQUIRES 
        unity 
        cmock
        main  # Our main component for testing
)

# Set up CMock configuration
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    -DUNITY_INCLUDE_CONFIG_H
    -DCMOCK_MEM_DYNAMIC
)

# Add compiler flags for testing
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-function
    -Wno-unused-variable
)
