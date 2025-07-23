#include "main.hpp"
#include "managers/rtos_manager.hpp"
#include "managers/I2CManager.hpp"
#include "managers/config_manager.hpp"
#include "managers/SensorManager.hpp"
#include "core/json_config_provider.hpp"
#include "sensors/AHT21Sensor.hpp"
#include "sensors/ENS160Sensor.hpp"
#include "network/wifi_manager.h"
#include "network/sntp_manager.h"
#include "network/http_client.h"
#include "network/http_server.h"
#include "network/secure_credentials.h"
#include "managers/health_monitor.hpp"
#include "network/health_server.hpp"
#include "managers/sensor_registry.hpp"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "esp_task_wdt.h"
#include <cstring>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <memory>
#include <string>

static const char* TAG = "SQLiteTest_CPP";
static std::shared_ptr<HealthMonitor> g_healthMonitor = nullptr;
static std::unique_ptr<HealthServer> g_healthServer = nullptr;
static std::unique_ptr<SensorRegistry> g_sensorRegistry = nullptr; // Added sensor registry

// Health callback function for HTTP server
extern "C" const char* health_callback(const char* request_data) {
    if (g_healthServer) {
        std::string response = g_healthServer->handleHealthRequest(request_data);
        
        // Allocate memory for response (caller will free)
        char* response_ptr = (char*)malloc(response.length() + 1);
        if (response_ptr) {
            strcpy(response_ptr, response.c_str());
            ESP_LOGI("HEALTH_CALLBACK", "Returning health data: %s", response_ptr);
            return response_ptr;
        }
    }
    
    // Default error response
    return strdup("{\"error\": \"Health monitoring not available\"}");
}

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
            
            if (!aht21Sensor->isInitialised()) {
                ESP_LOGW(TAG, "AHT21 sensor initialisation failed - continuing without sensor");
                aht21Sensor.reset(); // Remove the failed sensor
            } else {
                ESP_LOGI(TAG, "AHT21 sensor initialised successfully");
            }
        } catch (const std::exception& e) {
            ESP_LOGE(TAG, "AHT21 sensor initialisation exception: %s", e.what());
            ESP_LOGW(TAG, "AHT21 sensor initialisation failed - continuing without sensor");
            aht21Sensor.reset(); // Remove the failed sensor
        }
        esp_task_wdt_reset(); // Feed watchdog
        
        if (ens160Sensor) {
            ESP_LOGI(TAG, "Initialising ENS160 sensor...");
            try {
                ens160Sensor->initialiseFromConfig(configPath);
                
                if (!ens160Sensor->isInitialised()) {
                    ESP_LOGW(TAG, "ENS160 sensor initialisation failed - continuing without sensor");
                    ens160Sensor.reset(); // Remove the failed sensor
                } else {
                    ESP_LOGI(TAG, "ENS160 sensor initialised successfully");
                }
            } catch (const std::exception& e) {
                ESP_LOGE(TAG, "ENS160 sensor initialisation exception: %s", e.what());
                ESP_LOGW(TAG, "ENS160 sensor initialisation failed - continuing without sensor");
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
        
        // Initialise HTTP server
        ESP_LOGI(TAG, "Initialising HTTP server...");
        esp_err_t http_ret = http_server_init();
        if (http_ret == ESP_OK) {
            ESP_LOGI(TAG, "HTTP server initialised successfully");
            
            // Create configuration manager first
            auto configProvider = createConfigProvider(configPath);
            if (!configProvider) {
                ESP_LOGE(TAG, "Failed to create configuration provider");
                throw std::runtime_error("Configuration provider creation failed");
            }
            
            auto configManager = std::make_shared<ConfigManager>(std::move(configProvider));
            
            // Initialise sensor registry with configuration dependency
            ESP_LOGI(TAG, "Initialising sensor registry...");
            g_sensorRegistry = std::make_unique<SensorRegistry>(configManager);
            esp_err_t registry_ret = g_sensorRegistry->initialise();
            if (registry_ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to initialise sensor registry: %s", esp_err_to_name(registry_ret));
            } else {
                ESP_LOGI(TAG, "Sensor registry initialised - ID: %s", g_sensorRegistry->getSensorId().c_str());
                
                if (!g_sensorRegistry->isConfigured()) {
                    ESP_LOGW(TAG, "Sensor registry not fully configured - some features may not work");
                }
            }
            
            // Initialise health monitoring system
            ESP_LOGI(TAG, "Initialising health monitoring system...");
            g_healthMonitor = std::make_shared<HealthMonitor>("ESP32_Sensor_01");
            g_healthMonitor->initialise();
            g_healthServer = std::make_unique<HealthServer>(g_healthMonitor);
            // Register health callback with HTTP server
            esp_err_t callback_ret = http_server_register_health_callback(health_callback);
            if (callback_ret == ESP_OK) {
                ESP_LOGI(TAG, "Health callback registered successfully");
            } else {
                ESP_LOGE(TAG, "Failed to register health callback: %s", esp_err_to_name(callback_ret));
            }
        } else {
            ESP_LOGE(TAG, "Failed to initialise HTTP server: %s", esp_err_to_name(http_ret));
        }
        esp_task_wdt_reset(); // Feed watchdog

        // Create and initialise RTOS manager on heap to reduce stack usage
        auto rtosManager = std::make_unique<RTOSManager>();
        ESP_LOGI(TAG, "RTOS manager created");
        
        // Update health monitor with sensor status using existing sensors
        if (g_healthMonitor) {
            // Check sensor connections based on existing sensor instances
            bool aht21_connected = (aht21Sensor != nullptr && aht21Sensor->isInitialised());
            bool ens160_connected = (ens160Sensor != nullptr && ens160Sensor->isInitialised());
            
            g_healthMonitor->updateSensorStatus(aht21_connected, ens160_connected);
            ESP_LOGI(TAG, "Health monitor updated with sensor status: AHT21=%s, ENS160=%s", 
                      aht21_connected ? "connected" : "disconnected",
                      ens160_connected ? "connected" : "disconnected");
            
            if (!aht21_connected && !ens160_connected) {
                ESP_LOGW(TAG, "No sensors connected - system will operate in limited mode");
            }
            
            // Set initial sensor states
            g_healthMonitor->updateSensorStates("ready", "warm_up");
            ESP_LOGI(TAG, "Health monitor updated with sensor states: AHT21=ready, ENS160=warm_up");
        }
        
        // Create sensor manager and add sensors
        auto sensorManager = std::make_shared<SensorManager>(i2cManager);
        sensorManager->initialiseFromConfig(configPath);
        
        // Add sensors to the sensor manager
        if (aht21Sensor && aht21Sensor->isInitialised()) {
            sensorManager->addSensor("AHT21", aht21Sensor);
            ESP_LOGI(TAG, "AHT21 sensor added to sensor manager");
        }
        if (ens160Sensor && ens160Sensor->isInitialised()) {
            sensorManager->addSensor("ENS160", ens160Sensor);
            ESP_LOGI(TAG, "ENS160 sensor added to sensor manager");
            
        
            auto ens160Ptr = static_cast<ENS160Sensor*>(ens160Sensor.get());
            if (ens160Ptr) {

                ens160Ptr->setValidationThresholds(1, 5, 1, 10000, 1, 10000);
                ESP_LOGI(TAG, "ENS160 validation thresholds set");
            }
        }
        
        // Initialise RTOS manager with sensor manager
        ESP_ERROR_CHECK(rtosManager->initialise(i2cManager, sensorManager, configPath));
        
        // Set the health monitor to use the main's instance
        rtosManager->setHealthMonitor(g_healthMonitor);
        
        // Validate configuration before starting
        if (!rtosManager->validateConfiguration()) {
            ESP_LOGE(TAG, "Configuration validation failed - system cannot start");
            ESP_LOGE(TAG, "Please ensure data/settings.json exists and contains proper configuration values");
            ESP_LOGE(TAG, "Copy config/settings.template.jsonc to data/settings.json and configure with your actual values");
            throw std::runtime_error("Configuration validation failed");
        }
        
        // Start the RTOS manager
        ESP_LOGI(TAG, "Starting RTOS manager...");
        ESP_ERROR_CHECK(rtosManager->start());
        ESP_LOGI(TAG, "RTOS manager started successfully");
        
        // Send handshake to server to register this sensor
        ESP_LOGI(TAG, "Sending handshake to server...");
        const char* esp32Ip = wifi_manager_get_ip();
        const char* sensorId = g_sensorRegistry ? g_sensorRegistry->getSensorId().c_str() : "sensor_unknown";
        
        // Get server URL from sensor registry
        const char* serverUrl = g_sensorRegistry ? g_sensorRegistry->getServerUrl().c_str() : "";
        if (strlen(serverUrl) == 0) {
            ESP_LOGW(TAG, "Server URL not configured - skipping handshake");
        } else if (sendHandshakeToServer(sensorId, esp32Ip, serverUrl, 10000)) {
            ESP_LOGI(TAG, "Handshake successful - sensor %s registered with IP %s", sensorId, esp32Ip);
            ESP_LOGI(TAG, "Health data available at: http://your-server.local/api/%s", sensorId);
            
            // Mark as registered in the registry
            if (g_sensorRegistry) {
                // The registry will remember this sensor ID across reboots
                ESP_LOGI(TAG, "Sensor ID %s will be remembered across reboots", sensorId);
            }
        } else {
            ESP_LOGW(TAG, "Handshake failed - sensor may not be registered with server");
        }
        
        // Test health monitoring with some sample data
        if (g_healthMonitor) {
            ESP_LOGI(TAG, "Testing health monitoring system...");
            g_healthMonitor->recordSensorReading(true);
            g_healthMonitor->recordNetworkTransmission(true);
            ESP_LOGI(TAG, "Health monitoring test completed");
        }
        
        // Main loop
        ESP_LOGI(TAG, "System started successfully");
        
        TickType_t lastStatsTime = 0;
        
        while (rtosManager->isRunning()) {
            TickType_t currentTime = xTaskGetTickCount();
            
            // Feed watchdog every 5 seconds to prevent timeouts
            esp_task_wdt_reset();
            
            // Print statistics every 5 minutes
            if ((currentTime - lastStatsTime) >= pdMS_TO_TICKS(300000)) { // 5 minutes
                uint32_t readings, transmissions, errors;
                rtosManager->getStatistics(readings, transmissions, errors);
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
