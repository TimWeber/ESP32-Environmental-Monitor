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
#include "esp_log.h"

static const char* TAG = "UNIT_TESTS";

// Test function declarations - add new test functions here
extern void test_wifi_manager_init(void);
extern void test_wifi_manager_connect(void);
extern void test_http_client_post(void);
extern void test_sensor_health_tracking(void);

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
    
    UNITY_BEGIN();
    
    // Network module tests
    ESP_LOGI(TAG, "Running network module tests...");
    RUN_TEST(test_wifi_manager_init);
    RUN_TEST(test_wifi_manager_connect);
    RUN_TEST(test_http_client_post);
    
    // Sensor module tests
    ESP_LOGI(TAG, "Running sensor module tests...");
    RUN_TEST(test_sensor_health_tracking);
    
    // Add more test groups here as they are implemented
    
    UNITY_END();
    
    ESP_LOGI(TAG, "All unit tests completed");
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
