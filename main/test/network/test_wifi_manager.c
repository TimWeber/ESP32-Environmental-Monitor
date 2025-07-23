/*
 * Unit Tests for WiFi Manager Module
 * 
 * These tests verify the WiFi manager functionality using CMock to mock
 * ESP-IDF WiFi functions, allowing isolated testing of the logic.
 */

#include <string.h>
#include "unity.h"
#include "cmock.h"
#include "esp_log.h"

// Include the module under test
#include "network/wifi_manager.h"

// Include mock functions
#include "mocks/mockWifiFunctions.h"

static const char* TAG = "TEST_WIFI";

void setUp(void) {
    ESP_LOGI(TAG, "Setting up WiFi manager test");
    // Reset all mock states before each test
    mockWifiResetState();
}

void tearDown(void) {
    ESP_LOGI(TAG, "Tearing down WiFi manager test");
    // Clean up mock states after each test
    mockWifiResetState();
}

/**
 * @brief Test WiFi manager initialisation
 */
void testWifiManagerInit(void) {
    ESP_LOGI(TAG, "Testing WiFi manager initialisation");
    
    // Set up mock for successful WiFi initialisation
    mockWifiResetState();
    
    // Test the actual function
    esp_err_t result = wifi_manager_init();
    
    // Verify the result
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    ESP_LOGI(TAG, "WiFi manager init test passed");
}

/**
 * @brief Test WiFi connection logic
 */
void testWifiManagerConnect(void) {
    ESP_LOGI(TAG, "Testing WiFi connection logic");
    
    // Set up mock for successful WiFi connection
    mockWifiSetConnected(true);
    mockWifiSetSignalStrength(-45);
    
    // Test connection with valid credentials
    bool result = wifi_manager_connect_and_wait(30000);
    
    // Verify the connection was successful
    TEST_ASSERT_TRUE(result);
    
    ESP_LOGI(TAG, "WiFi connection test completed");
}

/**
 * @brief Test WiFi status checking
 */
void testWifiManagerStatus(void) {
    ESP_LOGI(TAG, "Testing WiFi status functions");
    
    // Set up mock for connected WiFi
    mockWifiSetConnected(true);
    mockWifiSetSignalStrength(-50);
    
    // Test is_connected function
    bool connected = wifi_manager_is_connected();
    TEST_ASSERT_TRUE(connected);
    
    // Test get_ip function
    const char* ip = wifi_manager_get_ip();
    
    // Basic sanity checks
    TEST_ASSERT_NOT_NULL(ip);
    
    ESP_LOGI(TAG, "WiFi status test completed");
}
