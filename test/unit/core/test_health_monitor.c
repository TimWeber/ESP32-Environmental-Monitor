/*
 * Unit Tests for Health Monitor
 * 
 * These tests verify health monitoring functionality, statistics collection,
 * and sensor status tracking for the ESP32 environmental monitoring system.
 */

#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "cmock.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include the modules under test
#include "core/health_monitor.hpp"
#include "network/health_server.hpp"

static const char* TAG = "TEST_HEALTH";

void setUp(void) {
    ESP_LOGI(TAG, "Setting up health monitor test");
}

void tearDown(void) {
    ESP_LOGI(TAG, "Tearing down health monitor test");
}

/**
 * @brief Test health monitor initialisation
 */
void testHealthMonitorInit(void) {
    ESP_LOGI(TAG, "Testing health monitor initialisation");
    
    try {
        auto healthMonitor = std::make_unique<HealthMonitor>("ESP32_Test_01");
        healthMonitor->initialise();
        
        std::string deviceId = healthMonitor->getDeviceId();
        TEST_ASSERT_EQUAL_STRING("ESP32_Test_01", deviceId.c_str());
        
        ESP_LOGI(TAG, "Health monitor initialisation test passed");
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Health monitor initialisation exception: %s", e.what());
        TEST_FAIL_MESSAGE("Health monitor initialisation should not throw exception");
    }
}

/**
 * @brief Test sensor status tracking
 */
void testHealthMonitorSensorStatus(void) {
    ESP_LOGI(TAG, "Testing health monitor sensor status tracking");
    
    try {
        auto healthMonitor = std::make_unique<HealthMonitor>("ESP32_Test_01");
        healthMonitor->initialise();
        
        healthMonitor->updateSensorStatus(true, false);
        
        bool aht21Connected = healthMonitor->isSensorConnected("aht21");
        bool ens160Connected = healthMonitor->isSensorConnected("ens160");
        
        TEST_ASSERT_TRUE(aht21Connected);
        TEST_ASSERT_FALSE(ens160Connected);
        
        ESP_LOGI(TAG, "Health monitor sensor status test passed");
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Health monitor sensor status exception: %s", e.what());
        TEST_FAIL_MESSAGE("Health monitor sensor status should not throw exception");
    }
}

/**
 * @brief Test statistics collection
 */
void testHealthMonitorStatistics(void) {
    ESP_LOGI(TAG, "Testing health monitor statistics collection");
    
    try {
        auto healthMonitor = std::make_unique<HealthMonitor>("ESP32_Test_01");
        healthMonitor->initialise();
        
        healthMonitor->incrementSensorReadings("aht21", true);
        healthMonitor->incrementSensorReadings("aht21", true);
        healthMonitor->incrementSensorReadings("aht21", false);
        
        uint32_t aht21Total = healthMonitor->getSensorReadingsTotal("aht21");
        uint32_t aht21Failed = healthMonitor->getSensorReadingsFailed("aht21");
        
        TEST_ASSERT_EQUAL(3, aht21Total);
        TEST_ASSERT_EQUAL(1, aht21Failed);
        
        ESP_LOGI(TAG, "Health monitor statistics test passed");
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Health monitor statistics exception: %s", e.what());
        TEST_FAIL_MESSAGE("Health monitor statistics should not throw exception");
    }
}

/**
 * @brief Test health data serialisation
 */
void testHealthMonitorDataSerialisation(void) {
    ESP_LOGI(TAG, "Testing health monitor data serialisation");
    
    try {
        auto healthMonitor = std::make_unique<HealthMonitor>("ESP32_Test_01");
        healthMonitor->initialise();
        
        healthMonitor->updateSensorStatus(true, true);
        healthMonitor->incrementSensorReadings("aht21", true);
        healthMonitor->incrementNetworkTransmissions(true);
        
        std::string healthJson = healthMonitor->getHealthDataJson();
        
        TEST_ASSERT_NOT_NULL(healthJson.c_str());
        TEST_ASSERT_GREATER_THAN(0, healthJson.length());
        
        ESP_LOGI(TAG, "Health monitor data serialisation test passed");
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Health monitor data serialisation exception: %s", e.what());
        TEST_FAIL_MESSAGE("Health monitor data serialisation should not throw exception");
    }
} 