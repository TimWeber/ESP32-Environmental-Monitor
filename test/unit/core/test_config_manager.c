/*
 * Unit Tests for Configuration Manager
 * 
 * These tests verify JSON configuration loading, validation, and error handling
 * for the ESP32 environmental monitoring system.
 */

#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "cmock.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include the modules under test
#include "core/config_manager.hpp"
#include "core/json_config_provider.hpp"

static const char* TAG = "TEST_CONFIG";

// Test configuration JSON strings
static const char* valid_config_json = R"({
    "i2c": {
        "sda_pin": 8,
        "scl_pin": 9,
        "frequency": 100000
    },
    "sensors": {
        "aht21": {
            "enabled": true,
            "address": "0x38",
            "read_interval_ms": 5000
        },
        "ens160": {
            "enabled": true,
            "address": "0x53",
            "read_interval_ms": 10000
        }
    },
    "network": {
        "wifi": {
            "ssid": "test_network",
            "password": "test_password"
        },
        "server": {
            "url": "http://test.server.com",
            "port": 8080
        }
    },
    "validation": {
        "temperature": {
            "min": -40.0,
            "max": 85.0
        },
        "humidity": {
            "min": 0.0,
            "max": 100.0
        },
        "aqi": {
            "min": 0,
            "max": 500
        }
    }
})";

static const char* invalid_config_json = R"({
    "i2c": {
        "sda_pin": "invalid",
        "scl_pin": 9
    }
})";

static const char* missing_required_config_json = R"({
    "sensors": {
        "aht21": {
            "enabled": true
        }
    }
})";

void setUp(void) {
    ESP_LOGI(TAG, "Setting up config manager test");
    // Reset all mocks before each test
    // CMock_Init();
}

void tearDown(void) {
    ESP_LOGI(TAG, "Tearing down config manager test");
    // Verify and destroy mocks after each test
    // CMock_Verify();
    // CMock_Destroy();
}

/**
 * @brief Test valid configuration loading
 */
void testConfigManagerValidConfig(void) {
    ESP_LOGI(TAG, "Testing configuration manager with valid config");
    
    try {
        // Create config provider with valid JSON
        auto configProvider = std::make_unique<JsonConfigProvider>(valid_config_json);
        auto configManager = std::make_unique<ConfigManager>(std::move(configProvider));
        
        // Test I2C configuration
        int sdaPin = configManager->getI2CPinSDA();
        int sclPin = configManager->getI2CPinSCL();
        int frequency = configManager->getI2CFrequency();
        
        TEST_ASSERT_EQUAL(8, sdaPin);
        TEST_ASSERT_EQUAL(9, sclPin);
        TEST_ASSERT_EQUAL(100000, frequency);
        
        // Test sensor configuration
        bool aht21Enabled = configManager->isSensorEnabled("aht21");
        bool ens160Enabled = configManager->isSensorEnabled("ens160");
        
        TEST_ASSERT_TRUE(aht21Enabled);
        TEST_ASSERT_TRUE(ens160Enabled);
        
        // Test validation thresholds
        float tempMin = configManager->getValidationThreshold("temperature", "min");
        float tempMax = configManager->getValidationThreshold("temperature", "max");
        
        TEST_ASSERT_EQUAL_FLOAT(-40.0f, tempMin);
        TEST_ASSERT_EQUAL_FLOAT(85.0f, tempMax);
        
        ESP_LOGI(TAG, "Valid configuration test passed");
        
    } catch (const std::exception& e) {
        TEST_FAIL_MESSAGE("Valid config should not throw exception");
    }
}

/**
 * @brief Test invalid configuration handling
 */
void testConfigManagerInvalidConfig(void) {
    ESP_LOGI(TAG, "Testing configuration manager with invalid config");
    
    try {
        // Create config provider with invalid JSON
        auto configProvider = std::make_unique<JsonConfigProvider>(invalid_config_json);
        auto configManager = std::make_unique<ConfigManager>(std::move(configProvider));
        
        // This should throw an exception or handle the error gracefully
        int sdaPin = configManager->getI2CPinSDA();
        
        // If we get here, the config manager should handle invalid data gracefully
        ESP_LOGI(TAG, "Invalid configuration handled gracefully");
        
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "Invalid configuration exception caught: %s", e.what());
        // Exception is expected for invalid configuration
    }
}

/**
 * @brief Test missing configuration handling
 */
void testConfigManagerMissingConfig(void) {
    ESP_LOGI(TAG, "Testing configuration manager with missing config");
    
    try {
        // Create config provider with missing required fields
        auto configProvider = std::make_unique<JsonConfigProvider>(missing_required_config_json);
        auto configManager = std::make_unique<ConfigManager>(std::move(configProvider));
        
        // Test that missing fields are handled gracefully
        int sdaPin = configManager->getI2CPinSDA();
        int sclPin = configManager->getI2CPinSCL();
        
        // Should return default values or handle gracefully
        ESP_LOGI(TAG, "Missing configuration handled gracefully: SDA=%d, SCL=%d", sdaPin, sclPin);
        
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "Missing configuration exception caught: %s", e.what());
        // Exception is acceptable for missing required fields
    }
}

/**
 * @brief Test configuration validation
 */
void testConfigManagerValidation(void) {
    ESP_LOGI(TAG, "Testing configuration validation");
    
    try {
        auto configProvider = std::make_unique<JsonConfigProvider>(valid_config_json);
        auto configManager = std::make_unique<ConfigManager>(std::move(configProvider));
        
        // Test validation thresholds
        float humidityMin = configManager->getValidationThreshold("humidity", "min");
        float humidityMax = configManager->getValidationThreshold("humidity", "max");
        int aqiMin = configManager->getValidationThreshold("aqi", "min");
        int aqiMax = configManager->getValidationThreshold("aqi", "max");
        
        TEST_ASSERT_EQUAL_FLOAT(0.0f, humidityMin);
        TEST_ASSERT_EQUAL_FLOAT(100.0f, humidityMax);
        TEST_ASSERT_EQUAL(0, aqiMin);
        TEST_ASSERT_EQUAL(500, aqiMax);
        
        ESP_LOGI(TAG, "Configuration validation test passed");
        
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "Configuration validation exception: %s", e.what());
        TEST_FAIL_MESSAGE("Configuration validation should not throw exception");
    }
}

/**
 * @brief Test sensor-specific configuration
 */
void testConfigManagerSensorConfig(void) {
    ESP_LOGI(TAG, "Testing sensor-specific configuration");
    
    try {
        auto configProvider = std::make_unique<JsonConfigProvider>(valid_config_json);
        auto configManager = std::make_unique<ConfigManager>(std::move(configProvider));
        
        // Test AHT21 configuration
        bool aht21Enabled = configManager->isSensorEnabled("aht21");
        int aht21Address = configManager->getSensorAddress("aht21");
        int aht21Interval = configManager->getSensorReadInterval("aht21");
        
        TEST_ASSERT_TRUE(aht21Enabled);
        TEST_ASSERT_EQUAL(0x38, aht21Address);
        TEST_ASSERT_EQUAL(5000, aht21Interval);
        
        // Test ENS160 configuration
        bool ens160Enabled = configManager->isSensorEnabled("ens160");
        int ens160Address = configManager->getSensorAddress("ens160");
        int ens160Interval = configManager->getSensorReadInterval("ens160");
        
        TEST_ASSERT_TRUE(ens160Enabled);
        TEST_ASSERT_EQUAL(0x53, ens160Address);
        TEST_ASSERT_EQUAL(10000, ens160Interval);
        
        ESP_LOGI(TAG, "Sensor-specific configuration test passed");
        
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "Sensor configuration exception: %s", e.what());
        TEST_FAIL_MESSAGE("Sensor configuration should not throw exception");
    }
}

/**
 * @brief Test network configuration
 */
void testConfigManagerNetworkConfig(void) {
    ESP_LOGI(TAG, "Testing network configuration");
    
    try {
        auto configProvider = std::make_unique<JsonConfigProvider>(valid_config_json);
        auto configManager = std::make_unique<ConfigManager>(std::move(configProvider));
        
        // Test WiFi configuration
        std::string ssid = configManager->getWiFiSSID();
        std::string password = configManager->getWiFiPassword();
        
        TEST_ASSERT_EQUAL_STRING("test_network", ssid.c_str());
        TEST_ASSERT_EQUAL_STRING("test_password", password.c_str());
        
        // Test server configuration
        std::string serverUrl = configManager->getServerURL();
        int serverPort = configManager->getServerPort();
        
        TEST_ASSERT_EQUAL_STRING("http://test.server.com", serverUrl.c_str());
        TEST_ASSERT_EQUAL(8080, serverPort);
        
        ESP_LOGI(TAG, "Network configuration test passed");
        
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "Network configuration exception: %s", e.what());
        TEST_FAIL_MESSAGE("Network configuration should not throw exception");
    }
}

/**
 * @brief Test configuration error handling
 */
void testConfigManagerErrorHandling(void) {
    ESP_LOGI(TAG, "Testing configuration error handling");
    
    // Test with null config provider
    try {
        auto configManager = std::make_unique<ConfigManager>(nullptr);
        TEST_FAIL_MESSAGE("Should not allow null config provider");
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "Null config provider exception caught: %s", e.what());
        // Exception is expected
    }
    
    // Test with empty JSON
    try {
        auto configProvider = std::make_unique<JsonConfigProvider>("{}");
        auto configManager = std::make_unique<ConfigManager>(std::move(configProvider));
        
        // Should handle empty config gracefully
        int sdaPin = configManager->getI2CPinSDA();
        ESP_LOGI(TAG, "Empty configuration handled gracefully: SDA=%d", sdaPin);
        
    } catch (const std::exception& e) {
        ESP_LOGI(TAG, "Empty configuration exception: %s", e.what());
        // Exception is acceptable for empty config
    }
    
    ESP_LOGI(TAG, "Configuration error handling test passed");
} 