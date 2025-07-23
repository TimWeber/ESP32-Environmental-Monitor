# ESP32 Environmental Monitoring System - Test Suite

This directory contains comprehensive unit tests for the ESP32 Environmental Monitoring System using Unity and CMock frameworks.

## Test Categories

### 1. Sensor Tests (`test_sensor_health.c`)
Tests for AHT21 and ENS160 sensor functionality:

- **`testAht21InitSuccess`** - Tests successful AHT21 sensor initialisation
- **`testAht21InitNoDevice`** - Tests AHT21 initialisation when no device is connected
- **`testAht21ReadData`** - Tests AHT21 data reading and validation
- **`testEns160InitSuccess`** - Tests successful ENS160 sensor initialisation
- **`testEns160InitNoDevice`** - Tests ENS160 initialisation when no device is connected
- **`testEns160ReadData`** - Tests ENS160 data reading and validation
- **`testSensorExceptionHandling`** - Tests robust exception handling for sensor failures
- **`testI2cManager`** - Tests I2C manager functionality
- **`testSensorHealthTracking`** - Tests sensor health monitoring

### 2. Configuration Tests (`test_config_manager.c`)
Tests for JSON configuration management:

- **`testConfigManagerValidConfig`** - Tests loading and parsing valid JSON configuration
- **`testConfigManagerInvalidConfig`** - Tests handling of invalid JSON configuration
- **`testConfigManagerMissingConfig`** - Tests handling of missing required configuration fields
- **`testConfigManagerValidation`** - Tests configuration validation logic
- **`testConfigManagerSensorConfig`** - Tests sensor-specific configuration parsing
- **`testConfigManagerNetworkConfig`** - Tests network configuration parsing
- **`testConfigManagerErrorHandling`** - Tests error handling for configuration issues

### 3. Health Monitor Tests (`test_health_monitor.c`)
Tests for health monitoring and statistics:

- **`testHealthMonitorInit`** - Tests health monitor initialisation
- **`testHealthMonitorSensorStatus`** - Tests sensor status tracking
- **`testHealthMonitorStatistics`** - Tests statistics collection and calculation
- **`testHealthMonitorDataSerialisation`** - Tests JSON health data generation

### 4. Network Tests (`test_wifi_manager.c`, `test_http_client.c`)
Tests for network functionality:

- **`test_wifi_manager_init`** - Tests WiFi manager initialisation
- **`test_wifi_manager_connect`** - Tests WiFi connection logic
- **`test_http_client_post`** - Tests HTTP client functionality

## Test Framework Setup

### Dependencies
- **Unity** - Unit testing framework
- **Custom Mocks** - Mock implementations for ESP-IDF functions
- **ESP-IDF** - ESP32 development framework

### Test Structure
```
test/
├── CMakeLists.txt              # Test build configuration
├── README.md                   # This documentation
└── unit/
    ├── test_main.c             # Main test runner
    ├── cmock_config.h          # Mock configuration
    ├── unity_config.h          # Unity configuration
    ├── network/                # Network module tests
    │   ├── test_wifi_manager.c
    │   └── test_http_client.c
    ├── sensors/                # Sensor module tests
    │   └── test_sensor_health.c
    ├── core/                   # Core module tests
    │   ├── test_config_manager.c
    │   └── test_health_monitor.c
    └── mocks/                  # Custom mock implementations
        ├── mockI2cFunctions.h   # I2C function mocks
        ├── mockI2cFunctions.c   # I2C mock implementations
        ├── mockWifiFunctions.h  # WiFi function mocks
        └── mockWifiFunctions.c  # WiFi mock implementations
```

## Running Tests

### **Option 1: Native Testing (Recommended for Unit Tests)**
Run tests on your PC without ESP32 hardware:

#### **Windows - Multiple Options**

**Option A: Simple ESP-IDF Testing (No External Compiler)**
```bash
# Navigate to test directory
cd test

# Build and prepare tests
run_tests_simple.bat

# Then run on hardware or simulation
idf.py flash monitor    # With ESP32 hardware
idf.py qemu            # With QEMU simulation (if installed)
```

**Option B: Native GCC Compilation**
```bash
# Navigate to test directory
cd test

# Run the native test runner (requires GCC)
run_native_tests.bat
```

**Option C: Native MSVC Compilation**
```bash
# Navigate to test directory
cd test

# Run with Visual Studio compiler
run_native_tests_msvc.bat
```

**Option D: WSL (Windows Subsystem for Linux)**
```bash
# In WSL terminal
cd test
chmod +x run_native_tests_wsl.sh
./run_native_tests_wsl.sh
```

#### **Linux/macOS**
```bash
# Navigate to test directory
cd test

# Install Unity and build tests
make setup

# Run tests
make test
```

#### **Manual Build**
```bash
# Navigate to test directory
cd test

# Install Unity test framework
git clone https://github.com/ThrowTheSwitch/Unity.git unity

# Build and run
gcc -Wall -Wextra -std=c99 -g -DNATIVE_TESTING \
    -I. -Iunit -Iunit/mocks -I../main -I../main/sensors -I../main/core -I../main/network \
    native_test_runner.c unit/*/*.c unit/mocks/*.c unity/src/unity.c \
    -o native_tests

# Run tests
./native_tests
```

### **Option 2: ESP32 Hardware Testing**
Run tests on actual ESP32 hardware:

```bash
# Navigate to test directory
cd test

# Build the test application
idf.py build

# Flash and monitor tests (requires ESP32 connected)
idf.py flash monitor
```

### **Option 3: Run from Root Directory**
```bash
# Build and flash test application from root
idf.py -p test build
idf.py -p test flash monitor
```

### Test Output
Tests provide detailed logging output:
```
I (1234) UNIT_TESTS: Starting unit tests for ESP32-C3 Environmental Monitor
I (1235) UNIT_TESTS: Running network module tests...
I (1236) TEST_WIFI: Testing WiFi manager initialisation
I (1237) TEST_WIFI: WiFi manager init test passed
I (1238) UNIT_TESTS: Running sensor module tests...
I (1239) TEST_SENSORS: Testing AHT21 sensor initialisation - success case
I (1240) TEST_SENSORS: AHT21 initialisation test passed
...
```

## Test Coverage

### Exception Handling
All tests include proper exception handling to ensure the system remains robust:
- Sensor communication failures
- Configuration parsing errors
- Network connection issues
- Memory allocation failures

### Edge Cases
Tests cover various edge cases:
- Missing or disconnected sensors
- Invalid configuration files
- Network connectivity issues
- Memory constraints
- Invalid data ranges

### Performance Testing
Some tests include performance validation:
- High-frequency sensor readings
- Memory usage tracking
- Network transmission statistics

## Mock System

### **Custom Mock Implementation**
Our test suite uses custom mock implementations rather than CMock generation:

#### **I2C Mocks** (`mockI2cFunctions.c`)
- **Purpose**: Mock I2C hardware communication for sensor testing
- **Key Functions**: `mockI2cMasterInit()`, `mockI2cMasterTransmit()`, `mockI2cMasterReceive()`
- **Control Functions**: `mockI2cSetAht21Connected()`, `mockI2cSetEns160Connected()`, `mockI2cResetAllDevices()`

#### **WiFi Mocks** (`mockWifiFunctions.c`)
- **Purpose**: Mock WiFi hardware for network testing
- **Key Functions**: `mockEspWifiInit()`, `mockEspWifiConnect()`, `mockEspWifiIsConnected()`
- **Control Functions**: `mockWifiSetConnected()`, `mockWifiSetSignalStrength()`, `mockWifiResetState()`

#### **Mock Usage Example**
```c
// Set up mock for connected sensor
mockI2cSetAht21Connected(true);

// Test sensor initialisation
AHT21Sensor sensor(i2cManager);
sensor.initialise();
TEST_ASSERT_TRUE(sensor.isInitialised());
```

### **Benefits of Custom Mocks**
- **Predictable**: Consistent responses for testing
- **Configurable**: Easy to change mock states between tests
- **Hardware Independent**: Tests run without real sensors or WiFi
- **Fast**: No hardware delays or communication overhead
- **Reliable**: Tests don't depend on external factors

## Adding New Tests

### 1. Create Test File
Create a new test file in the appropriate directory:
```c
// test/unit/your_module/testYourFunction.c
#include "unity.h"
#include "esp_log.h"
#include "mocks/mockI2cFunctions.h"  // Include relevant mocks

static const char* TAG = "TEST_YOUR_MODULE";

void setUp(void) {
    ESP_LOGI(TAG, "Setting up test");
}

void tearDown(void) {
    ESP_LOGI(TAG, "Tearing down test");
}

void testYourFunction(void) {
    ESP_LOGI(TAG, "Testing your function");
    
    // Your test logic here
    TEST_ASSERT_TRUE(true);
    
    ESP_LOGI(TAG, "Test passed");
}
```

### 2. Add to CMakeLists.txt
Add your test file to `test/CMakeLists.txt`:
```cmake
SRCS 
    "unit/test_main.c"
    "unit/your_module/testYourFunction.c"
    # ... other files
```

### 3. Add to Test Runner
Add your test function declaration and execution to `test/unit/test_main.c`:
```c
// Add declaration
extern void testYourFunction(void);

// Add to test runner
RUN_TEST(testYourFunction);
```

## Test Best Practices

### 1. Exception Safety
Always wrap test logic in try-catch blocks:
```c
try {
    // Test logic
    TEST_ASSERT_TRUE(result);
} catch (const std::exception& e) {
    ESP_LOGE(TAG, "Test exception: %s", e.what());
    TEST_FAIL_MESSAGE("Test should not throw exception");
}
```

### 2. Resource Cleanup
Ensure proper cleanup in tearDown():
```c
void tearDown(void) {
    // Clean up any resources allocated in setUp()
    // Reset mocks
    // Free memory
}
```

### 3. Meaningful Assertions
Use specific assertions:
```c
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_GREATER_OR_EQUAL(min_value, actual);
TEST_ASSERT_NOT_NULL(pointer);
TEST_ASSERT_EQUAL_STRING(expected_string, actual_string);
```

### 4. Comprehensive Logging
Provide detailed logging for debugging:
```c
ESP_LOGI(TAG, "Testing specific functionality");
ESP_LOGI(TAG, "Test passed with result: %d", result);
ESP_LOGW(TAG, "Expected failure handled: %s", error_message);
```

## Continuous Integration

The test suite is designed to run in CI/CD pipelines:
- All tests must pass before deployment
- Code coverage reporting
- Performance regression testing
- Memory leak detection

## Troubleshooting

### Common Issues

1. **Test fails with I2C errors**
   - Ensure no physical sensors are connected during testing
   - Check that mocks are properly configured

2. **Configuration tests fail**
   - Verify JSON syntax in test configuration strings
   - Check that all required fields are present

3. **Memory allocation failures**
   - Increase heap size in sdkconfig
   - Check for memory leaks in test code

4. **Network tests timeout**
   - Ensure WiFi credentials are not required for tests
   - Mock network functions appropriately

### Debug Mode
Enable debug logging for detailed test output:
```bash
idf.py menuconfig
# Navigate to Component config -> Log output -> Default log verbosity -> Debug
```

## Contributing

When adding new functionality to the main application:
1. Write corresponding unit tests
2. Ensure all tests pass
3. Update this documentation
4. Add appropriate exception handling
5. Include edge case testing
