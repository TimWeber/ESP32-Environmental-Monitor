/*
 * ESP32-C3 Environmental Monitor - Unit Test Main
 * 
 * This file provides the main entry point for unit tests using Unity and CMock.
 * Tests are organized by module (network, sensors, etc.) for better maintainability.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "unity.h"
#include "unity_config.h"
#include "test_result_tracker.h"
#include "esp_log.h"

static const char* TAG = "UNIT_TESTS";

// Test wrapper function to properly track results
#define RUN_TEST_WITH_TRACKING(test_func) do { \
    testResultStartTest(#test_func); \
    test_func(); \
    if (testResultGetCurrentTestResult()) { \
        testResultRecordSuccess(#test_func); \
    } else { \
        testResultRecordFailure(#test_func, "Test assertion failed", __LINE__, __FILE__); \
    } \
} while(0)

// Test function declarations - add new test functions here
extern void testWifiManagerInit(void);
extern void testWifiManagerConnect(void);
extern void testHttpClientPost(void);
extern void testSensorHealthTracking(void);

// Sensor tests
extern void testAht21InitSuccess(void);
extern void testAht21InitNoDevice(void);
extern void testAht21ReadData(void);
extern void testEns160InitSuccess(void);
extern void testEns160InitNoDevice(void);
extern void testEns160ReadData(void);
extern void testSensorExceptionHandling(void);
extern void testI2cManager(void);
extern void testDemonstrateFailure(void);

// Configuration tests
extern void testConfigManagerValidConfig(void);
extern void testConfigManagerInvalidConfig(void);
extern void testConfigManagerMissingConfig(void);
extern void testConfigManagerValidation(void);
extern void testConfigManagerSensorConfig(void);
extern void testConfigManagerNetworkConfig(void);
extern void testConfigManagerErrorHandling(void);

// Health monitor tests
extern void testHealthMonitorInit(void);
extern void testHealthMonitorSensorStatus(void);
extern void testHealthMonitorStatistics(void);
extern void testHealthMonitorDataSerialisation(void);

void setUp(void) {
    // Set up function called before each test
    ESP_LOGI(TAG, "Setting up test environment");
}

void tearDown(void) {
    // Tear down function called after each test
    ESP_LOGI(TAG, "Cleaning up test environment");
}

/**
 * @brief Main test runner function
 */
void run_all_tests(void) {
    ESP_LOGI(TAG, "Starting unit tests for ESP32-C3 Environmental Monitor");
    ESP_LOGI(TAG, "========================================================");
    
    // Initialize test result tracking
    testResultInit();
    
    UNITY_BEGIN();
    
    // Network module tests
    ESP_LOGI(TAG, "Running network module tests...");
    RUN_TEST_WITH_TRACKING(testWifiManagerInit);
    RUN_TEST_WITH_TRACKING(testWifiManagerConnect);
    RUN_TEST_WITH_TRACKING(testHttpClientPost);
    
    // Sensor module tests
    ESP_LOGI(TAG, "Running sensor module tests...");
    RUN_TEST_WITH_TRACKING(testSensorHealthTracking);
    RUN_TEST_WITH_TRACKING(testAht21InitSuccess);
    RUN_TEST_WITH_TRACKING(testAht21InitNoDevice);
    RUN_TEST_WITH_TRACKING(testAht21ReadData);
    RUN_TEST_WITH_TRACKING(testEns160InitSuccess);
    RUN_TEST_WITH_TRACKING(testEns160InitNoDevice);
    RUN_TEST_WITH_TRACKING(testEns160ReadData);
    RUN_TEST_WITH_TRACKING(testSensorExceptionHandling);
    RUN_TEST_WITH_TRACKING(testI2cManager);
    RUN_TEST_WITH_TRACKING(testDemonstrateFailure);
    
    // Configuration module tests
    ESP_LOGI(TAG, " Running configuration module tests...");
    RUN_TEST_WITH_TRACKING(testConfigManagerValidConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerInvalidConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerMissingConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerValidation);
    RUN_TEST_WITH_TRACKING(testConfigManagerSensorConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerNetworkConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerErrorHandling);
    
    // Health monitor module tests
    ESP_LOGI(TAG, " Running health monitor module tests...");
    RUN_TEST_WITH_TRACKING(testHealthMonitorInit);
    RUN_TEST_WITH_TRACKING(testHealthMonitorSensorStatus);
    RUN_TEST_WITH_TRACKING(testHealthMonitorStatistics);
    RUN_TEST_WITH_TRACKING(testHealthMonitorDataSerialisation);
    
    UNITY_END();
    
    // Print test summary
    ESP_LOGI(TAG, "========================================================");
    testResultPrintSummary();
    ESP_LOGI(TAG, "✅ All unit tests completed!");
}

/**
 * @brief Test application main function
 */
void app_main(void) {
    ESP_LOGI(TAG, "ESP32-C3 Environmental Monitor - Unit Test Suite");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    
    // Give the system a moment to stabilise
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Run all tests
    run_all_tests();
    
    // Keep the test application running
    while (1) {
        ESP_LOGI(TAG, "Tests complete. System idle.");
        vTaskDelay(pdMS_TO_TICKS(60000)); // Wait 1 minute
    }
}
