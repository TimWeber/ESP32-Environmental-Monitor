#include "main.hpp"

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <memory>

// ESP-IDF includes
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

// Core system components
#include "core/core.hpp"

// Sensor components
#include "sensors/AHT21Sensor.hpp"
#include "sensors/ENS160Sensor.hpp"

// Legacy C modules
extern "C" {
    #include "network/wifi_manager.h"
    #include "network/sntp_manager.h"
    #include "network/secure_credentials.h"
}

static const char* TAG = "SQLiteTest_CPP";

/**
 * @brief Main application entry point
 */
extern "C" void app_main() {
    ESP_LOGI(TAG, "=== SQLiteTest C++ Application Starting ===");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    
    // Initialise NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialise SPIFFS
    ESP_LOGI(TAG, "Initialising SPIFFS...");
    esp_vfs_spiffs_conf_t spiffsConf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t spiffsRet = esp_vfs_spiffs_register(&spiffsConf);
    if (spiffsRet != ESP_OK) {
        ESP_LOGW(TAG, "Failed to mount SPIFFS: %s", esp_err_to_name(spiffsRet));
        ESP_LOGW(TAG, "Continuing without SPIFFS - using default settings");
    } else {
        size_t total = 0, used = 0;
        esp_err_t infoRet = esp_spiffs_info("storage", &total, &used);
        if (infoRet == ESP_OK) {
            ESP_LOGI(TAG, "SPIFFS: total=%zu, used=%zu", total, used);
        }
    }
    
    try {
        // Use embedded settings from data/settings.json at runtime
        const char* configPath = nullptr;
        
        if (spiffsRet == ESP_OK) {
            configPath = "/spiffs/settings.json";
            ESP_LOGI(TAG, "SPIFFS mounted successfully, checking config: %s", configPath);
            
            // Check if SPIFFS config file exists
            FILE* spiffsFile = fopen(configPath, "rb");
            if (spiffsFile) {
                fclose(spiffsFile);
                ESP_LOGI(TAG, "SPIFFS config file exists and will be used: %s", configPath);
            } else {
                ESP_LOGW(TAG, "SPIFFS config file not found: %s", configPath);
                configPath = nullptr;
                ESP_LOGW(TAG, "Using default settings (no config file)");
            }
        } else {
            ESP_LOGW(TAG, "SPIFFS not available, using default settings");
        }

        // Add main task to watchdog timer early
        esp_task_wdt_add(xTaskGetCurrentTaskHandle());

        // Initialise secure credentials system
        ESP_LOGI(TAG, "Initialising secure credentials system...");
        ESP_ERROR_CHECK(secure_credentials_init());
        esp_task_wdt_reset(); // Feed watchdog
        
        // Load credentials from config file if not already set
        if (!secure_credentials_has_wifi() || !secure_credentials_has_server()) {
            ESP_LOGI(TAG, "Loading credentials from config file...");
            esp_err_t configResult = secure_credentials_load_from_config(configPath);
            if (configResult != ESP_OK) {
                ESP_LOGW(TAG, "Failed to load credentials from config file: %s", esp_err_to_name(configResult));
                ESP_LOGW(TAG, "Continuing without config file - using default settings");
            }
        }
        esp_task_wdt_reset(); // Feed watchdog

        // Initialise I2C manager with config
        auto i2cManager = std::make_shared<I2CManager>();
        i2cManager->initialiseFromConfig(configPath);
        esp_task_wdt_reset(); // Feed watchdog
        
        // Scan I2C bus to check for devices
        ESP_LOGI(TAG, "Scanning I2C bus for devices...");
        int devicesFound = i2cManager->scanBus();
        if (devicesFound == 0) {
            ESP_LOGW(TAG, "No I2C devices found - check wiring and power");
        }
        esp_task_wdt_reset(); // Feed watchdog
        
        // Create sensors
        auto aht21Sensor = std::make_shared<AHT21Sensor>(*i2cManager);
        auto ens160Sensor = std::make_shared<ENS160Sensor>(*i2cManager);
        
        // Initialise sensors with config
        ESP_LOGI(TAG, "Initialising AHT21 sensor...");
        try {
            aht21Sensor->initialiseFromConfig(configPath);
            ESP_LOGI(TAG, "AHT21 sensor initialized successfully");
        } catch (const std::exception& e) {
            ESP_LOGE(TAG, "Failed to initialize AHT21 sensor: %s", e.what());
            ESP_LOGE(TAG, "Check if AHT21 sensor is properly connected to I2C bus");
            aht21Sensor.reset(); // Remove the failed sensor
        }
        esp_task_wdt_reset(); // Feed watchdog
        
        if (ens160Sensor) {
            ESP_LOGI(TAG, "Initialising ENS160 sensor...");
            try {
                ens160Sensor->initialiseFromConfig(configPath);
                ESP_LOGI(TAG, "ENS160 sensor initialized successfully");
            } catch (const std::exception& e) {
                ESP_LOGE(TAG, "Failed to initialize ENS160 sensor: %s", e.what());
                ESP_LOGE(TAG, "Check if ENS160 sensor is properly connected to I2C bus");
                ens160Sensor.reset(); // Remove the failed sensor
            }
        }
        esp_task_wdt_reset(); // Feed watchdog
        
        // Initialise WiFi
        ESP_LOGI(TAG, "Initialising WiFi...");
        ESP_ERROR_CHECK(wifi_manager_init_from_config(configPath));
        if (!wifi_manager_connect_and_wait_from_config(configPath)) {
            ESP_LOGE(TAG, "Failed to connect to WiFi");
            throw std::runtime_error("WiFi connection failed");
        }
        ESP_LOGI(TAG, "WiFi connected, IP: %s", wifi_manager_get_ip());
        
        // Initialise SNTP
        ESP_LOGI(TAG, "Initialising SNTP...");
        sntp_manager_init();
        
        // Create and initialise RTOS manager
        RTOSManager rtosManager;
        ESP_ERROR_CHECK(rtosManager.initialise(i2cManager, aht21Sensor, ens160Sensor, configPath));
        
        // Validate configuration before starting
        if (!rtosManager.validateConfiguration()) {
            ESP_LOGE(TAG, "Configuration validation failed - system cannot start");
            ESP_LOGE(TAG, "Please ensure data/settings.json exists and contains proper configuration values");
            ESP_LOGE(TAG, "Copy config/settings.template.jsonc to data/settings.json and configure with your actual values");
            throw std::runtime_error("Configuration validation failed");
        }
        
        // Start the RTOS manager
        ESP_ERROR_CHECK(rtosManager.start());
        
        // Main loop
        ESP_LOGI(TAG, "System started successfully");
        
        while (rtosManager.isRunning()) {
            // Feed watchdog every 5 seconds to prevent timeouts
            esp_task_wdt_reset();
            
            // Print statistics every 10 seconds
            uint32_t readings, transmissions, errors;
            rtosManager.getStatistics(readings, transmissions, errors);
            
            // Print system statistics every 5 minutes
            static TickType_t lastStatsTime = 0;
            TickType_t currentTime = xTaskGetTickCount();
            if ((currentTime - lastStatsTime) >= pdMS_TO_TICKS(300000)) { // 5 minutes
                ESP_LOGI(TAG, "System Statistics: readings=%lu, transmissions=%lu, errors=%lu, uptime=%lu ms",
                         readings, transmissions, errors, pdTICKS_TO_MS(currentTime));
                lastStatsTime = currentTime;
            }
            
            // Feed watchdog again before delay
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(5000));  // 5 second loop
        }
        
        // Remove main task from watchdog timer
        esp_task_wdt_delete(xTaskGetCurrentTaskHandle());
        
        ESP_LOGI(TAG, "RTOS manager stopped, shutting down");
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Application failed: %s", e.what());
        ESP_LOGE(TAG, "Restarting in 10 seconds...");
        vTaskDelay(pdMS_TO_TICKS(10000));
        esp_restart();
    }
}
