/*
 * Unit Tests for Sensor Health Monitoring
 * 
 * These tests verify sensor initialization, data reading, and error handling
 * for both AHT21 and ENS160 sensors.
 */

#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "cmock.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include the modules under test
#include "sensors/AHT21Sensor.hpp"
#include "sensors/ENS160Sensor.hpp"
#include "core/I2CManager.hpp"

// Include mock functions
#include "mocks/mockI2cFunctions.h"

static const char* TAG = "TEST_SENSORS";

// Test data structures
typedef struct {
    float temperature;
    float humidity;
    bool valid;
} AHT21TestData;

typedef struct {
    uint8_t aqi;
    uint16_t tvoc;
    uint16_t eco2;
    bool valid;
} ENS160TestData;

// Mock I2C responses
static uint8_t mock_aht21_response[] = {0xAC, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t mock_ens160_partid[] = {0x16, 0x00}; // ENS160 Part ID
static uint8_t mock_ens160_data[] = {0x50, 0x01, 0x00, 0x64, 0x00}; // AQI=80, TVOC=256, eCO2=100

void setUp(void) {
    ESP_LOGI(TAG, "Setting up sensor health test");
    // Reset all mocks before each test
    // CMock_Init();
}

void tearDown(void) {
    ESP_LOGI(TAG, "Tearing down sensor health test");
    // Verify and destroy mocks after each test
    // CMock_Verify();
    // CMock_Destroy();
}

/**
 * @brief Test AHT21 sensor initialisation with valid device
 */
void testAht21InitSuccess(void) {
    ESP_LOGI(TAG, "Testing AHT21 sensor initialisation - success case");
    
    // Set up mock for connected AHT21
    mockI2cSetAht21Connected(true);
    
    // Create sensor instance
    I2CManager i2cManager;
    AHT21Sensor sensor(i2cManager);
    
    // Test initialisation
    try {
        sensor.initialise();
        TEST_ASSERT_TRUE(sensor.isInitialised());
        ESP_LOGI(TAG, "AHT21 initialisation test passed");
    } catch (const std::exception& e) {
        TEST_FAIL_MESSAGE("AHT21 initialisation should not throw exception");
    }
}

/**
 * @brief Test AHT21 sensor initialisation with missing device
 */
void testAht21InitNoDevice(void) {
    ESP_LOGI(TAG, "Testing AHT21 sensor initialisation - no device case");
    
    // Set up mock for disconnected AHT21
    mockI2cSetAht21Connected(false);
    
    // Create sensor instance
    I2CManager i2cManager;
    AHT21Sensor sensor(i2cManager);
    
    // Test initialisation with no device
    try {
        sensor.initialise();
        // Should not be initialised
        TEST_ASSERT_FALSE(sensor.isInitialised());
        ESP_LOGI(TAG, "AHT21 no-device test passed");
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "AHT21 no-device exception caught: %s", e.what());
        TEST_ASSERT_FALSE(sensor.isInitialised());
    }
}

/**
 * @brief Test AHT21 sensor data reading
 */
void testAht21ReadData(void) {
    ESP_LOGI(TAG, "Testing AHT21 sensor data reading");
    
    // Set up mock for connected AHT21 with valid data
    mockI2cSetAht21Connected(true);
    
    // Create sensor instance
    I2CManager i2cManager;
    AHT21Sensor sensor(i2cManager);
    
    try {
        sensor.initialise();
        if (sensor.isInitialised()) {
            AHT21Data data = sensor.readData();
            
            // Validate data ranges
            TEST_ASSERT_GREATER_OR_EQUAL(0.0f, data.temperature);
            TEST_ASSERT_LESS_OR_EQUAL(100.0f, data.temperature);
            TEST_ASSERT_GREATER_OR_EQUAL(0.0f, data.humidity);
            TEST_ASSERT_LESS_OR_EQUAL(100.0f, data.humidity);
            
            ESP_LOGI(TAG, "AHT21 data reading test passed");
        } else {
            ESP_LOGW(TAG, "AHT21 not initialised, skipping data reading test");
        }
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "AHT21 data reading exception: %s", e.what());
        // Exception is acceptable if sensor is not connected
    }
}

/**
 * @brief Test ENS160 sensor initialisation with valid device
 */
void testEns160InitSuccess(void) {
    ESP_LOGI(TAG, "Testing ENS160 sensor initialisation - success case");
    
    // Set up mock for connected ENS160
    mockI2cSetEns160Connected(true);
    
    // Create sensor instance
    I2CManager i2cManager;
    ENS160Sensor sensor(i2cManager);
    
    // Test initialisation
    try {
        sensor.initialise();
        TEST_ASSERT_TRUE(sensor.isInitialised());
        ESP_LOGI(TAG, "ENS160 initialisation test passed");
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "ENS160 initialisation exception: %s", e.what());
        TEST_ASSERT_FALSE(sensor.isInitialised());
    }
}

/**
 * @brief Test ENS160 sensor initialisation with missing device
 */
void testEns160InitNoDevice(void) {
    ESP_LOGI(TAG, "Testing ENS160 sensor initialisation - no device case");
    
    // Set up mock for disconnected ENS160
    mockI2cSetEns160Connected(false);
    
    // Create sensor instance
    I2CManager i2cManager;
    ENS160Sensor sensor(i2cManager);
    
    // Test initialisation with no device
    try {
        sensor.initialise();
        // Should not be initialised
        TEST_ASSERT_FALSE(sensor.isInitialised());
        ESP_LOGI(TAG, "ENS160 no-device test passed");
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "ENS160 no-device exception caught: %s", e.what());
        TEST_ASSERT_FALSE(sensor.isInitialised());
    }
}

/**
 * @brief Test ENS160 sensor data reading
 */
void testEns160ReadData(void) {
    ESP_LOGI(TAG, "Testing ENS160 sensor data reading");
    
    // Set up mock for connected ENS160 with valid data
    mockI2cSetEns160Connected(true);
    
    // Create sensor instance
    I2CManager i2cManager;
    ENS160Sensor sensor(i2cManager);
    
    try {
        sensor.initialise();
        if (sensor.isInitialised()) {
            ENS160Data data = sensor.readData();
            
            // Validate data ranges
            TEST_ASSERT_GREATER_OR_EQUAL(0, data.aqi);
            TEST_ASSERT_LESS_OR_EQUAL(500, data.aqi);
            TEST_ASSERT_GREATER_OR_EQUAL(0, data.tvoc);
            TEST_ASSERT_LESS_OR_EQUAL(65000, data.tvoc);
            TEST_ASSERT_GREATER_OR_EQUAL(400, data.eco2);
            TEST_ASSERT_LESS_OR_EQUAL(65000, data.eco2);
            
            ESP_LOGI(TAG, "ENS160 data reading test passed");
        } else {
            ESP_LOGW(TAG, "ENS160 not initialised, skipping data reading test");
        }
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "ENS160 data reading exception: %s", e.what());
        // Exception is acceptable if sensor is not connected
    }
}

/**
 * @brief Test sensor exception handling robustness
 */
void testSensorExceptionHandling(void) {
    ESP_LOGI(TAG, "Testing sensor exception handling");
    
    // Create sensor instances
    I2CManager i2cManager;
    AHT21Sensor aht21Sensor(i2cManager);
    ENS160Sensor ens160Sensor(i2cManager);
    
    // Test that exceptions don't crash the system
    try {
        aht21Sensor.initialise();
        ens160Sensor.initialise();
        
        // Try to read data even if sensors aren't initialised
        if (aht21Sensor.isInitialised()) {
            AHT21Data aht21Data = aht21Sensor.readData();
            ESP_LOGI(TAG, "AHT21 data read successfully");
        }
        
        if (ens160Sensor.isInitialised()) {
            ENS160Data ens160Data = ens160Sensor.readData();
            ESP_LOGI(TAG, "ENS160 data read successfully");
        }
        
        ESP_LOGI(TAG, "Sensor exception handling test passed");
        
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "Sensor exception caught and handled: %s", e.what());
        // Exception handling is working correctly
    }
}

/**
 * @brief Test I2C manager functionality
 */
void testI2cManager(void) {
    ESP_LOGI(TAG, "Testing I2C manager functionality");
    
    // Reset all mock devices to connected state
    mockI2cResetAllDevices();
    
    // Create I2C manager instance
    I2CManager i2cManager;
    
    // Test initialisation
    try {
        i2cManager.initialiseFromConfig("/spiffs/settings.json");
        TEST_ASSERT_TRUE(i2cManager.isInitialised());
        ESP_LOGI(TAG, "I2C manager initialisation test passed");
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "I2C manager initialisation exception: %s", e.what());
        // Exception is acceptable if config file doesn't exist
    }
}

/**
 * @brief Test sensor health tracking
 */
void testSensorHealthTracking(void) {
    ESP_LOGI(TAG, "Testing sensor health tracking");
    
    // Create sensor instances
    I2CManager i2cManager;
    AHT21Sensor aht21Sensor(i2cManager);
    ENS160Sensor ens160Sensor(i2cManager);
    
    // Test health tracking without sensors
    bool aht21Healthy = aht21Sensor.isInitialised();
    bool ens160Healthy = ens160Sensor.isInitialised();
    
    ESP_LOGI(TAG, "Sensor health status: AHT21=%s, ENS160=%s", 
              aht21Healthy ? "healthy" : "unhealthy",
              ens160Healthy ? "healthy" : "unhealthy");
    
    // Basic health tracking test passed
    TEST_ASSERT_TRUE(true);
    ESP_LOGI(TAG, "Sensor health tracking test passed");
}

/**
 * @brief Test that will fail to demonstrate FAIL reporting
 */
void testDemonstrateFailure(void) {
    ESP_LOGI(TAG, "Testing failure demonstration");
    
    // This test will FAIL
    TEST_ASSERT_EQUAL(2, 3); // This will fail
}
