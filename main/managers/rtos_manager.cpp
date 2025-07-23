#include "managers/rtos_manager.hpp"
#include "managers/I2CManager.hpp"
#include "sensors/AHT21Sensor.hpp"
#include "sensors/ENS160Sensor.hpp"
#include "network/http_client.h"
#include "network/secure_credentials.h"
#include "network/wifi_manager.h"
#include "network/health_server.hpp"
#include "cJSON.h"
#include <cstring>
#include <exception>

// Event group bits for task synchronization
#define SENSOR_DATA_READY_BIT BIT0
#define SYSTEM_ERROR_BIT       BIT1

// Network command types
enum NetworkCommand {
    CMD_SEND_SENSOR_DATA,
    CMD_SYSTEM_STATUS,
    CMD_ERROR_REPORT
};

// Constructor
RTOSManager::RTOSManager()
    : sensorTaskHandle_(nullptr),
      networkTaskHandle_(nullptr),
      monitorTaskHandle_(nullptr),
      watchdogTaskHandle_(nullptr),
      heartbeatTaskHandle_(nullptr),
      sensorDataQueue_(nullptr),
      networkCmdQueue_(nullptr),
      systemEventGroup_(nullptr),
      sensorMutex_(nullptr),
      systemRunning_(false),
      sensorReadingsCount_(0),
      networkTransmissionsCount_(0),
      networkErrorsCount_(0),
      sensorReadingIntervalMs_(5000),       // Default 5 seconds
      networkTransmissionIntervalMs_(30000), // Default 30 seconds
      monitorReportIntervalMs_(60000),      // Default 60 seconds
      intervalsAreSame_(false),             // Default to different intervals
      heartbeatIntervalSeconds_(60),        // Default 60 seconds
      sensorQueueSize_(10),                 // Default 10 items
      detailedLoggingEnabled_(false),
      sleepModeEnabled_(false),
      networkMaxRetries_(3),
      networkRetryDelayMs_(1000),
      networkTimeoutMs_(10000),
      i2cSdaPin_(8),
      i2cSclPin_(9),
      aqiMin_(1),
      aqiMax_(5),
      tvocMin_(1),
      tvocMax_(10000),
      eco2Min_(200),
      eco2Max_(10000),
      heartbeatTaskRunning_(false),
      healthMonitor_(std::make_shared<HealthMonitor>("ESP32_Sensor_01")) {
    
    // Initialize sensor health status
    aht21Health_.sensorName = "AHT21";
    aht21Health_.expectedId = AHT21_EXPECTED_STATUS_MASK;
    
    ens160Health_.sensorName = "ENS160";
    ens160Health_.expectedId = ENS160_EXPECTED_PART_ID;
    
    ESP_LOGI(TAG, "RTOSManager created");
}

// Destructor
RTOSManager::~RTOSManager() {
    stop();
    ESP_LOGI(TAG, "RTOSManager destroyed");
}

esp_err_t RTOSManager::initialise(std::shared_ptr<I2CManager> i2cManager,
                                 std::shared_ptr<SensorManager> sensorManager,
                                 const char* configPath) {
    ESP_LOGI(TAG, "Initialising RTOS manager");
    
    // Check if already running
    if (systemRunning_.load()) {
        ESP_LOGW(TAG, "RTOS manager already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Store sensor objects
    i2cManager_ = i2cManager;
    sensorManager_ = sensorManager;
    
    // Load configuration
    if (!loadConfiguration(configPath)) {
        ESP_LOGW(TAG, "Failed to load configuration, using defaults");
    }
    
    // Initialise watchdog
    initialiseWatchdog();
    
    // Create event group for inter-task communication
    systemEventGroup_ = xEventGroupCreate();
    if (!systemEventGroup_) {
        ESP_LOGE(TAG, "Failed to create system event group");
        return ESP_ERR_NO_MEM;
    }
    
    // Create sensor data queue
    sensorDataQueue_ = xQueueCreate(sensorQueueSize_, sizeof(SensorData));
    if (!sensorDataQueue_) {
        ESP_LOGE(TAG, "Failed to create sensor data queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create network command queue
    networkCmdQueue_ = xQueueCreate(NETWORK_CMD_QUEUE_SIZE, sizeof(NetworkCommand));
    if (!networkCmdQueue_) {
        ESP_LOGE(TAG, "Failed to create network command queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create sensor mutex
    sensorMutex_ = xSemaphoreCreateMutex();
    if (!sensorMutex_) {
        ESP_LOGE(TAG, "Failed to create sensor mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Mark system as initialised but not running yet
    systemRunning_.store(false);
    
    // Initialize heartbeat monitoring
    heartbeatTaskRunning_ = false;
    
    // Initialize health monitoring
    healthMonitor_->initialise();
    healthServer_ = std::make_unique<HealthServer>(healthMonitor_);
    
    ESP_LOGI(TAG, "RTOSManager initialised successfully");
    return ESP_OK;
}

esp_err_t RTOSManager::start() {
    if (systemRunning_.load()) {
        ESP_LOGW(TAG, "System already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting RTOS tasks");
    
    // Set system running flag
    systemRunning_.store(true);
    
    // Create sensor task
    BaseType_t result = xTaskCreatePinnedToCore(
        sensorTask,
        "SensorTask",
        SENSOR_TASK_STACK_SIZE,
        this,
        SENSOR_TASK_PRIORITY,
        &sensorTaskHandle_,
        0  // Run on core 0
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor task");
        systemRunning_.store(false);
        return ESP_ERR_NO_MEM;
    }
    
    // Create network task
    result = xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        NETWORK_TASK_STACK_SIZE,
        this,
        NETWORK_TASK_PRIORITY,
        &networkTaskHandle_,
        0  // Run on core 0
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create network task");
        vTaskDelete(sensorTaskHandle_);
        sensorTaskHandle_ = nullptr;
        systemRunning_.store(false);
        return ESP_ERR_NO_MEM;
    }
    
    // Create monitor task
    result = xTaskCreatePinnedToCore(
        monitorTask,
        "MonitorTask",
        MONITOR_TASK_STACK_SIZE,
        this,
        MONITOR_TASK_PRIORITY,
        &monitorTaskHandle_,
        0  // Core 0
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create monitor task");
        vTaskDelete(sensorTaskHandle_);
        vTaskDelete(networkTaskHandle_);
        sensorTaskHandle_ = nullptr;
        networkTaskHandle_ = nullptr;
        systemRunning_.store(false);
        return ESP_ERR_NO_MEM;
    }
    
    // Create heartbeat task
    result = xTaskCreatePinnedToCore(
        heartbeatTask,
        "HeartbeatTask",
        HEARTBEAT_TASK_STACK_SIZE,
        this,
        HEARTBEAT_TASK_PRIORITY,
        &heartbeatTaskHandle_,
        0  // Core 0
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create heartbeat task");
        vTaskDelete(sensorTaskHandle_);
        vTaskDelete(networkTaskHandle_);
        vTaskDelete(monitorTaskHandle_);
        sensorTaskHandle_ = nullptr;
        networkTaskHandle_ = nullptr;
        monitorTaskHandle_ = nullptr;
        systemRunning_.store(false);
        return ESP_ERR_NO_MEM;
    }
    
    // Mark heartbeat task as running
    heartbeatTaskRunning_.store(true);
    
    // Watchdog task disabled to avoid conflicts
    watchdogTaskHandle_ = nullptr;
    
    ESP_LOGI(TAG, "All RTOS tasks started successfully");
    return ESP_OK;
}

void RTOSManager::stop() {
    if (!systemRunning_.load()) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping RTOS tasks");
    systemRunning_.store(false);
    
    // Mark heartbeat task as stopped
    heartbeatTaskRunning_.store(false);
    
    // Wait for tasks to finish
    if (sensorTaskHandle_) {
        vTaskDelete(sensorTaskHandle_);
        sensorTaskHandle_ = nullptr;
    }
    if (networkTaskHandle_) {
        vTaskDelete(networkTaskHandle_);
        networkTaskHandle_ = nullptr;
    }
    if (monitorTaskHandle_) {
        vTaskDelete(monitorTaskHandle_);
        monitorTaskHandle_ = nullptr;
    }
    if (heartbeatTaskHandle_) {
        vTaskDelete(heartbeatTaskHandle_);
        heartbeatTaskHandle_ = nullptr;
    }
    if (watchdogTaskHandle_) {
        vTaskDelete(watchdogTaskHandle_);
        watchdogTaskHandle_ = nullptr;
    }
    
    // Clean up resources
    if (sensorDataQueue_) {
        vQueueDelete(sensorDataQueue_);
        sensorDataQueue_ = nullptr;
    }
    if (networkCmdQueue_) {
        vQueueDelete(networkCmdQueue_);
        networkCmdQueue_ = nullptr;
    }
    if (systemEventGroup_) {
        vEventGroupDelete(systemEventGroup_);
        systemEventGroup_ = nullptr;
    }
    if (sensorMutex_) {
        vSemaphoreDelete(sensorMutex_);
        sensorMutex_ = nullptr;
    }
    
    ESP_LOGI(TAG, "RTOS tasks stopped");
}

void RTOSManager::getStatistics(uint32_t& readingsCount, uint32_t& transmissionsCount, uint32_t& errorsCount) const {
    readingsCount = sensorReadingsCount_.load();
    transmissionsCount = networkTransmissionsCount_.load();
    errorsCount = networkErrorsCount_.load();
}

bool RTOSManager::validateConfiguration() const {
    if (!configManager_) {
        ESP_LOGE(TAG, "Configuration validation failed: No config manager available");
        return false;
    }
    
    return configManager_->validateConfiguration();
}

void RTOSManager::initialiseWatchdog() {
    esp_task_wdt_config_t wdtConfig = {
        .timeout_ms = 10000,  // 10 second timeout 
        .idle_core_mask = (1 << 0),  // Watch CPU0 idle task
        .trigger_panic = true  // Panic on timeout
    };
    esp_task_wdt_init(&wdtConfig);
    ESP_LOGI(TAG, "Watchdog initialised with 10 second timeout");
}

void RTOSManager::feedWatchdog() {
    esp_task_wdt_reset();
}

void RTOSManager::handleSystemError(const char* errorMsg) {
            ESP_LOGE(TAG, "System error: %s", errorMsg);
        xEventGroupSetBits(systemEventGroup_, SYSTEM_ERROR_BIT);
}

bool RTOSManager::loadConfiguration(const char* configPath) {
    if (!configPath) {
        ESP_LOGW(TAG, "No config path provided, using default parameters");
        return false;
    }
    
    ESP_LOGI(TAG, "Loading configuration from: %s", configPath);
    
    // Debug: Check if file exists
    FILE* testFile = fopen(configPath, "rb");
    if (!testFile) {
        ESP_LOGE(TAG, "Config file does not exist: %s", configPath);
        return false;
    } else {
        ESP_LOGI(TAG, "Config file exists and is readable: %s", configPath);
        fclose(testFile);
    }
    
    // Create config provider based on file type
    auto provider = createConfigProvider(std::string(configPath));
    if (!provider) {
        ESP_LOGW(TAG, "Failed to create config provider for: %s", configPath);
        return false;
    }
    
    // Create config manager with the provider
    configManager_ = std::make_unique<ConfigManager>(std::move(provider));
    
    // Load all configuration sections
    bool success = true;
    success &= configManager_->loadDataCollectionConfig();
    success &= configManager_->loadDataTransmissionConfig();
    success &= configManager_->loadConnectivityConfig();
    success &= configManager_->loadSystemMonitoringConfig();
    
    if (success) {
        // Apply loaded configuration to RTOSManager member variables
        sensorReadingIntervalMs_ = configManager_->getSensorReadingIntervalMs();
        networkTransmissionIntervalMs_ = configManager_->getNetworkTransmissionIntervalMs();
        monitorReportIntervalMs_ = configManager_->getMonitorReportIntervalMs();
        
        // Check if intervals are the same
        intervalsAreSame_ = (sensorReadingIntervalMs_ == networkTransmissionIntervalMs_);
        if (intervalsAreSame_) {
            ESP_LOGI(TAG, "Read and post intervals are the same (%lu ms) - ensuring read before post", 
                     sensorReadingIntervalMs_);
        }
        heartbeatIntervalSeconds_ = configManager_->getHeartbeatIntervalSeconds();
        sensorQueueSize_ = configManager_->getSensorQueueSize();
        detailedLoggingEnabled_ = configManager_->isDetailedLoggingEnabled();
        sleepModeEnabled_ = configManager_->isSleepModeEnabled();
        networkMaxRetries_ = configManager_->getNetworkMaxRetries();
        networkRetryDelayMs_ = configManager_->getNetworkRetryDelayMs();
        networkTimeoutMs_ = configManager_->getNetworkTimeoutMs();
        serverUrl_ = configManager_->getServerUrl();
        
        // Set validation thresholds on ENS160 sensor if available
        // Note: RTTI is disabled, so we can't use dynamic_cast
        // The validation thresholds will be set when the sensor is added to the manager
        if (sensorManager_ && sensorManager_->hasSensor("ENS160")) {
            ESP_LOGI(TAG, "ENS160 sensor found in sensor manager");
        }
        
        ESP_LOGI(TAG, "Configuration loaded successfully using protocol-agnostic manager");
    } else {
        ESP_LOGW(TAG, "Failed to load some configuration sections");
    }
    
    return success;
}

template<typename F>
bool RTOSManager::checkSensorHealth(SensorHealthStatus& healthStatus, F idCheckFunction) {
    TickType_t currentTime = xTaskGetTickCount();
    uint32_t actualId = 0;
    bool checkSuccess = false;
    
    try {
        // Call the provided function to get sensor ID
        actualId = idCheckFunction();
        checkSuccess = true;
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "%s ID check exception: %s", healthStatus.sensorName.c_str(), e.what());
        checkSuccess = false;
    }
    
    // Update health status
    healthStatus.lastCheckTime = pdTICKS_TO_MS(currentTime);
    healthStatus.actualId = actualId;
    
    // Check if ID matches expected value
    bool idValid = (actualId == healthStatus.expectedId);
    
            if (checkSuccess && idValid) {
            // Sensor is healthy
            if (!healthStatus.isHealthy) {
                ESP_LOGI(TAG, "%s recovered - ID check passed (0x%08lX)", 
                         healthStatus.sensorName.c_str(), (unsigned long)actualId);
            }
            healthStatus.isHealthy = true;
            healthStatus.consecutiveFailures = 0;
            return true;
        } else {
            // Sensor is unhealthy
            healthStatus.consecutiveFailures++;
            healthStatus.isHealthy = false;
            
            if (checkSuccess) {
                ESP_LOGW(TAG, "%s unhealthy - ID mismatch: expected 0x%08lX, got 0x%08lX (failures: %d)", 
                         healthStatus.sensorName.c_str(), (unsigned long)healthStatus.expectedId, (unsigned long)actualId, 
                         healthStatus.consecutiveFailures);
            } else {
                ESP_LOGW(TAG, "%s unhealthy - ID check failed (failures: %d)", 
                         healthStatus.sensorName.c_str(), healthStatus.consecutiveFailures);
            }
            
            return false;
        }
}

// Static task functions
void RTOSManager::sensorTask(void* parameter) {
    RTOSManager* manager = static_cast<RTOSManager*>(parameter);
    ESP_LOGI(TAG, "Sensor task started");
    
    // Add this task to watchdog timer
    esp_task_wdt_add(xTaskGetCurrentTaskHandle());
    
    TickType_t lastReadingTime = 0;
    
    while (manager->systemRunning_.load()) {
        TickType_t currentTime = xTaskGetTickCount();
        
        // Debug: Log timing information
        static TickType_t lastDebugTime = 0;
        if ((currentTime - lastDebugTime) >= pdMS_TO_TICKS(30000)) { // Every 30 seconds
            ESP_LOGI(TAG, "Sensor task debug: currentTime=%lu, lastReadingTime=%lu, interval=%lu, timeSinceLast=%lu",
                     currentTime, lastReadingTime, manager->sensorReadingIntervalMs_,
                     pdTICKS_TO_MS(currentTime - lastReadingTime));
            lastDebugTime = currentTime;
        }
        
        if ((currentTime - lastReadingTime) >= pdMS_TO_TICKS(manager->sensorReadingIntervalMs_)) {
            ESP_LOGI(TAG, "Sensor task: Starting sensor reading cycle");
            
            // Acquire sensor mutex
            if (xSemaphoreTake(manager->sensorMutex_, pdMS_TO_TICKS(1000)) == pdTRUE) {
                try {
                    // Read all sensors using SensorManager
                    if (manager->sensorManager_) {
                        ESP_LOGI(TAG, "Sensor task: Reading all sensors via SensorManager...");
                        SensorData sensor_data = manager->sensorManager_->readAllSensors();
                        sensor_data.timestamp = pdTICKS_TO_MS(currentTime);
                        sensor_data.isNewData = true; // Set this to true for new readings
                        
                        // Log sensor data
                        ESP_LOGI(TAG, "Sensor data: %zu fields, %zu valid sensors", 
                                  sensor_data.values.size(), sensor_data.getValidSensors().size());
                        
                        // Log individual sensor values
                        for (const auto& [field, value] : sensor_data.values) {
                            ESP_LOGI(TAG, "  %s: %.2f", field.c_str(), value);
                        }
                        
                        // Set environmental compensation for ENS160 if we have temperature/humidity
                        if (sensor_data.hasValue("temperature") && sensor_data.hasValue("humidity")) {
                            float temp = sensor_data.getValue("temperature");
                            float humidity = sensor_data.getValue("humidity");
                            
                            // Find ENS160 sensor and set compensation
                            auto ens160Reading = manager->sensorManager_->readSensor("ENS160");
                            if (ens160Reading.valid) {
                                // Note: This would need to be updated to work with the new interface
                                // For now, we'll skip environmental compensation
                                ESP_LOGD(TAG, "Environmental compensation: T=%.1f°C, H=%.1f%%", temp, humidity);
                            }
                        }
                        
                        // Record sensor reading success/failure for health monitoring
                        manager->healthMonitor_->recordSensorReading(true);
                        
                        // Send to network task
                        if (xQueueSend(manager->sensorDataQueue_, &sensor_data, pdMS_TO_TICKS(1000)) != pdTRUE) {
                            ESP_LOGW(TAG, "Failed to send sensor data to network task");
                        }
                        
                    } else {
                        ESP_LOGW(TAG, "SensorManager not available");
                        manager->healthMonitor_->recordSensorReading(false);
                    }
                    
                } catch (const std::exception& e) {
                    ESP_LOGE(TAG, "Sensor reading failed: %s", e.what());
                    manager->healthMonitor_->recordSensorReading(false);
                }
                
                xSemaphoreGive(manager->sensorMutex_);
            }
            
            lastReadingTime = currentTime;
        }
        
        // Feed watchdog
        manager->feedWatchdog();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "Sensor task finished");
    vTaskDelete(nullptr);
}

void RTOSManager::networkTask(void* parameter) {
    RTOSManager* manager = static_cast<RTOSManager*>(parameter);
    ESP_LOGI(TAG, "Network task started");
    
    // Add this task to watchdog timer
    esp_task_wdt_add(xTaskGetCurrentTaskHandle());
    
    // If intervals are the same, add a small delay to ensure sensor task runs first
    if (manager->intervalsAreSame_) {
        ESP_LOGI(TAG, "Intervals are the same - adding delay to ensure sensor task runs first");
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay
    }
    
    while (manager->systemRunning_.load()) {
        SensorData sensor_data;
        
        // Feed watchdog before waiting for data
        manager->feedWatchdog();
        
        // Wait for sensor data with very short timeout to allow frequent watchdog feeding
        TickType_t waitTime = pdMS_TO_TICKS(1000);  // 1 second timeout maximum
        
        // Debug: Log network task status
        static TickType_t lastNetworkDebugTime = 0;
        TickType_t currentNetworkTime = xTaskGetTickCount();
        if ((currentNetworkTime - lastNetworkDebugTime) >= pdMS_TO_TICKS(30000)) { // Every 30 seconds
            ESP_LOGI(TAG, "Network task debug: Waiting for sensor data...");
            lastNetworkDebugTime = currentNetworkTime;
        }
        
        if (xQueueReceive(manager->sensorDataQueue_, &sensor_data, waitTime) == pdTRUE) {
            ESP_LOGI(TAG, "Network task: Received sensor data from queue");
            // Only process if this is new data
            if (sensor_data.isNewData) {
                ESP_LOGI(TAG, "Network task: Processing new sensor data");
                // Feed watchdog before starting network operation
                manager->feedWatchdog();
                
                // Convert to database format
                sensorDatabaseData_t dbData = {};
                
                // Extract sensor data from unified format
                if (sensor_data.isSensorValid("AHT21")) {
                    dbData.temperature = sensor_data.getValue("temperature");
                    dbData.humidity = sensor_data.getValue("humidity");
                    dbData.aht21Valid = true;
                }
                
                if (sensor_data.isSensorValid("ENS160")) {
                    dbData.aqi = (uint8_t)sensor_data.getValue("aqi");
                    dbData.tvoc = (uint16_t)sensor_data.getValue("tvoc");
                    dbData.eco2 = (uint16_t)sensor_data.getValue("eco2");
                    dbData.ens160Valid = true;
                }
                
                // Send to database with retry logic and configurable parameters
                bool success = false;
                uint32_t retryCount = 0;
                
                while (!success && retryCount <= manager->networkMaxRetries_) {
                    if (retryCount > 0) {
                        ESP_LOGW(TAG, "Retrying network transmission (attempt %lu/%lu)", 
                                 (unsigned long)retryCount, (unsigned long)manager->networkMaxRetries_);
                        vTaskDelay(pdMS_TO_TICKS(manager->networkRetryDelayMs_));
                    }
                    
                    try {
                        // Use consolidated HTTP function with all configurable parameters
                        const char* serverUrl = manager->serverUrl_.empty() ? nullptr : manager->serverUrl_.c_str();
                        success = sendSensorDataToDatabase(&dbData, 
                                                         manager->networkTimeoutMs_, 
                                                         serverUrl,
                                                         manager->configManager_->getAqiMin(),
                                                         manager->configManager_->getAqiMax(),
                                                         manager->configManager_->getTvocMin(),
                                                         manager->configManager_->getTvocMax(),
                                                         manager->configManager_->getEco2Min(),
                                                         manager->configManager_->getEco2Max());
                    } catch (const std::exception& e) {
                        ESP_LOGE(TAG, "Network operation failed with exception: %s", e.what());
                        success = false;
                    }
                    
                    if (!success) {
                        retryCount++;
                        if (retryCount <= manager->networkMaxRetries_) {
                            ESP_LOGW(TAG, "Network transmission failed, will retry in %lu ms", 
                                     (unsigned long)manager->networkRetryDelayMs_);
                        }
                    }
                }
                
                if (success) {
                    manager->networkTransmissionsCount_.fetch_add(1);
                    manager->healthMonitor_->recordNetworkTransmission(true);
                    if (manager->detailedLoggingEnabled_) {
                        ESP_LOGI(TAG, "Sensor data sent to database successfully");
                        ESP_LOGI(TAG, "Network transmission successful - watchdog fed");
                    }
                    // Feed watchdog when data is successfully transmitted
                    manager->feedWatchdog();
                } else {
                    manager->networkErrorsCount_.fetch_add(1);
                    manager->healthMonitor_->recordNetworkTransmission(false);
                    ESP_LOGE(TAG, "Failed to send sensor data to database after %lu retries", 
                             (unsigned long)manager->networkMaxRetries_);
                }
                
                // Feed watchdog after network operation
                manager->feedWatchdog();
            } else {
                // No data received within timeout - normal with 15-minute intervals
                static uint32_t timeoutCount = 0;
                timeoutCount++;
                if (timeoutCount % 60 == 0) {  // Log every 60 timeouts
                    ESP_LOGI(TAG, "No sensor data received (normal with 15-minute intervals) - timeout count: %lu", timeoutCount);
                }
            }
            
            // Feed watchdog regularly
            manager->feedWatchdog();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    ESP_LOGI(TAG, "Network task finished");
    vTaskDelete(nullptr);
}

void RTOSManager::monitorTask(void* parameter) {
    RTOSManager* manager = static_cast<RTOSManager*>(parameter);
    ESP_LOGI(TAG, "Monitor task started");
    
    // Add this task to watchdog timer
    esp_task_wdt_add(xTaskGetCurrentTaskHandle());
    
    TickType_t lastReportTime = 0;
    TickType_t lastFeedTime = 0;
    uint32_t watchdogFeedCount = 0;
    
    while (manager->systemRunning_.load()) {
        TickType_t currentTime = xTaskGetTickCount();
        
        // Feed watchdog first
        manager->feedWatchdog();
        watchdogFeedCount++;
        
        // Yield to other tasks
        taskYIELD();
        
        if ((currentTime - lastReportTime) >= pdMS_TO_TICKS(manager->monitorReportIntervalMs_)) {
            uint32_t readings, transmissions, errors;
            manager->getStatistics(readings, transmissions, errors);
            
            ESP_LOGI(TAG, "System Status: readings=%lu, transmissions=%lu, errors=%lu, uptime=%lu ms, watchdog_feeds=%lu",
                     readings, transmissions, errors, pdTICKS_TO_MS(currentTime), watchdogFeedCount);
            
            // Check for system errors 
            EventBits_t bits = xEventGroupGetBits(manager->systemEventGroup_);
            if (bits & SYSTEM_ERROR_BIT) {
                ESP_LOGW(TAG, "System error detected");
                xEventGroupClearBits(manager->systemEventGroup_, SYSTEM_ERROR_BIT);
            }
            
            lastReportTime = currentTime;
        }
        
        // Log every 5 seconds for debugging
        if ((currentTime - lastFeedTime) >= pdMS_TO_TICKS(5000)) {
            ESP_LOGI(TAG, "Monitor task feeding watchdog (uptime: %lu ms, feeds: %lu)", 
                     pdTICKS_TO_MS(currentTime), watchdogFeedCount);
            lastFeedTime = currentTime;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "Monitor task finished");
    vTaskDelete(nullptr);
}

void RTOSManager::heartbeatTask(void* parameter) {
    RTOSManager* manager = static_cast<RTOSManager*>(parameter);
    ESP_LOGI(TAG, "Heartbeat task started");
    
    
    TickType_t lastHeartbeatTime = 0;
    uint32_t heartbeatCount = 0;
    
    while (manager->heartbeatTaskRunning_.load()) {
        TickType_t currentTime = xTaskGetTickCount();
        
        if ((currentTime - lastHeartbeatTime) >= pdMS_TO_TICKS(manager->heartbeatIntervalSeconds_ * 1000)) {
            heartbeatCount++;
            ESP_LOGI(TAG, "=== Heartbeat Check #%lu ===", heartbeatCount);
            
            bool systemHealthy = true;
            
            // Check all sensors health using SensorManager
            if (manager->sensorManager_) {
                manager->sensorManager_->checkAllSensorHealth();
                auto healthStatuses = manager->sensorManager_->getAllSensorHealth();
                
                for (const auto& [sensorName, health] : healthStatuses) {
                    if (!health.isHealthy) {
                        systemHealthy = false;
                        ESP_LOGW(TAG, "%s sensor unhealthy - consecutive failures: %d", 
                                 sensorName.c_str(), health.consecutiveFailures);
                    } else {
                        ESP_LOGI(TAG, "%s sensor healthy", sensorName.c_str());
                    }
                }
            }
            
            // Log overall system health
            if (systemHealthy) {
                ESP_LOGI(TAG, "All sensors healthy - system OK");
            } else {
                ESP_LOGW(TAG, "System unhealthy - some sensors failing");
                
                // Check if any sensor has exceeded failure threshold
                bool criticalFailure = false;
                if (manager->sensorManager_) {
                    auto healthStatuses = manager->sensorManager_->getAllSensorHealth();
                    for (const auto& [sensorName, health] : healthStatuses) {
                        if (health.consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
                            ESP_LOGE(TAG, "CRITICAL: %s sensor failure threshold exceeded", sensorName.c_str());
                            criticalFailure = true;
                        }
                    }
                }
                
                if (criticalFailure) {
                    ESP_LOGE(TAG, "CRITICAL: Sensor failure threshold exceeded - triggering system restart");
                    
                    // Trigger system restart
                    vTaskDelay(pdMS_TO_TICKS(1000));  // Give time for logs
                    esp_restart();
                }
            }
            
            lastHeartbeatTime = currentTime;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
    }
    
    ESP_LOGI(TAG, "Heartbeat task finished");
    vTaskDelete(nullptr);
}

void RTOSManager::watchdogTask(void* parameter) {
    RTOSManager* manager = static_cast<RTOSManager*>(parameter);
    ESP_LOGI(TAG, "Watchdog task started");
    
    // Add this task to watchdog timer
    esp_task_wdt_add(xTaskGetCurrentTaskHandle());
    
    TickType_t lastFeedTime = 0;
    uint32_t watchdogFeedCount = 0;
    
    while (manager->systemRunning_.load()) {
        TickType_t currentTime = xTaskGetTickCount();
        
        // Feed watchdog every second
        manager->feedWatchdog();
        watchdogFeedCount++;
        
        // Log every 10 seconds
        if ((currentTime - lastFeedTime) >= pdMS_TO_TICKS(10000)) {
            ESP_LOGI(TAG, "Watchdog task feeding watchdog (uptime: %lu ms, feeds: %lu)", 
                     pdTICKS_TO_MS(currentTime), watchdogFeedCount);
            lastFeedTime = currentTime;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "Watchdog task finished");
    vTaskDelete(nullptr);
}

// Move constructor and assignment operator
RTOSManager::RTOSManager(RTOSManager&& other) noexcept
    : sensorTaskHandle_(other.sensorTaskHandle_),
      networkTaskHandle_(other.networkTaskHandle_),
      monitorTaskHandle_(other.monitorTaskHandle_),
      watchdogTaskHandle_(other.watchdogTaskHandle_),
      heartbeatTaskHandle_(other.heartbeatTaskHandle_),
      sensorDataQueue_(other.sensorDataQueue_),
      networkCmdQueue_(other.networkCmdQueue_),
      systemEventGroup_(other.systemEventGroup_),
      sensorMutex_(other.sensorMutex_),
      systemRunning_(other.systemRunning_.load()),
      sensorReadingsCount_(other.sensorReadingsCount_.load()),
      networkTransmissionsCount_(other.networkTransmissionsCount_.load()),
      networkErrorsCount_(other.networkErrorsCount_.load()),
      i2cManager_(std::move(other.i2cManager_)),
      sensorManager_(std::move(other.sensorManager_)),
      configManager_(std::move(other.configManager_)),
      sensorReadingIntervalMs_(other.sensorReadingIntervalMs_),
      networkTransmissionIntervalMs_(other.networkTransmissionIntervalMs_),
      monitorReportIntervalMs_(other.monitorReportIntervalMs_),
      intervalsAreSame_(other.intervalsAreSame_),
      heartbeatIntervalSeconds_(other.heartbeatIntervalSeconds_),
      sensorQueueSize_(other.sensorQueueSize_),
      detailedLoggingEnabled_(other.detailedLoggingEnabled_),
      sleepModeEnabled_(other.sleepModeEnabled_),
      networkMaxRetries_(other.networkMaxRetries_),
      networkRetryDelayMs_(other.networkRetryDelayMs_),
      networkTimeoutMs_(other.networkTimeoutMs_),
      serverUrl_(std::move(other.serverUrl_)),
      i2cSdaPin_(other.i2cSdaPin_),
      i2cSclPin_(other.i2cSclPin_),
      aqiMin_(other.aqiMin_),
      aqiMax_(other.aqiMax_),
      tvocMin_(other.tvocMin_),
      tvocMax_(other.tvocMax_),
      eco2Min_(other.eco2Min_),
      eco2Max_(other.eco2Max_),
      aht21Health_(other.aht21Health_),
      ens160Health_(other.ens160Health_),
      heartbeatTaskRunning_(other.heartbeatTaskRunning_.load()) {
    
    // Reset other's handles
    other.sensorTaskHandle_ = nullptr;
    other.networkTaskHandle_ = nullptr;
    other.monitorTaskHandle_ = nullptr;
    other.watchdogTaskHandle_ = nullptr;
    other.heartbeatTaskHandle_ = nullptr;
            other.sensorDataQueue_ = nullptr;
        other.networkCmdQueue_ = nullptr;
        other.systemEventGroup_ = nullptr;
        other.sensorMutex_ = nullptr;
                other.systemRunning_.store(false);
        other.heartbeatTaskRunning_.store(false);
    
    ESP_LOGI(TAG, "RTOSManager moved");
}

RTOSManager& RTOSManager::operator=(RTOSManager&& other) noexcept {
    if (this != &other) {
        // Stop current system
        stop();
        
        // Move resources
        sensorTaskHandle_ = other.sensorTaskHandle_;
        networkTaskHandle_ = other.networkTaskHandle_;
        monitorTaskHandle_ = other.monitorTaskHandle_;
        watchdogTaskHandle_ = other.watchdogTaskHandle_;
        heartbeatTaskHandle_ = other.heartbeatTaskHandle_;
        sensorDataQueue_ = other.sensorDataQueue_;
        networkCmdQueue_ = other.networkCmdQueue_;
        systemEventGroup_ = other.systemEventGroup_;
        sensorMutex_ = other.sensorMutex_;
        systemRunning_.store(other.systemRunning_.load());
        sensorReadingsCount_.store(other.sensorReadingsCount_.load());
        networkTransmissionsCount_.store(other.networkTransmissionsCount_.load());
        networkErrorsCount_.store(other.networkErrorsCount_.load());
        i2cManager_ = std::move(other.i2cManager_);
        sensorManager_ = std::move(other.sensorManager_);
        configManager_ = std::move(other.configManager_);
        sensorReadingIntervalMs_ = other.sensorReadingIntervalMs_;
        networkTransmissionIntervalMs_ = other.networkTransmissionIntervalMs_;
        monitorReportIntervalMs_ = other.monitorReportIntervalMs_;
        intervalsAreSame_ = other.intervalsAreSame_;
        heartbeatIntervalSeconds_ = other.heartbeatIntervalSeconds_;
        sensorQueueSize_ = other.sensorQueueSize_;
        detailedLoggingEnabled_ = other.detailedLoggingEnabled_;
        sleepModeEnabled_ = other.sleepModeEnabled_;
        networkMaxRetries_ = other.networkMaxRetries_;
        networkRetryDelayMs_ = other.networkRetryDelayMs_;
        networkTimeoutMs_ = other.networkTimeoutMs_;
        serverUrl_ = std::move(other.serverUrl_);
        i2cSdaPin_ = other.i2cSdaPin_;
        i2cSclPin_ = other.i2cSclPin_;
        aqiMin_ = other.aqiMin_;
        aqiMax_ = other.aqiMax_;
        tvocMin_ = other.tvocMin_;
        tvocMax_ = other.tvocMax_;
        eco2Min_ = other.eco2Min_;
        eco2Max_ = other.eco2Max_;
        aht21Health_ = other.aht21Health_;
        ens160Health_ = other.ens160Health_;
        heartbeatTaskRunning_.store(other.heartbeatTaskRunning_.load());
        
        // Clear other's handles
        other.sensorTaskHandle_ = nullptr;
        other.networkTaskHandle_ = nullptr;
        other.monitorTaskHandle_ = nullptr;
        other.watchdogTaskHandle_ = nullptr;
        other.heartbeatTaskHandle_ = nullptr;
        other.sensorDataQueue_ = nullptr;
        other.networkCmdQueue_ = nullptr;
        other.systemEventGroup_ = nullptr;
        other.sensorMutex_ = nullptr;
        other.systemRunning_.store(false);
    }
    return *this;
} 