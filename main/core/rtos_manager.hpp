#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "config_manager.hpp"
#include "health_monitor.hpp"
#include <memory>
#include <atomic>
#include <string>

// Forward declarations
class I2CManager;
class AHT21Sensor;
class ENS160Sensor;
class HealthServer;

/**
 * @brief Sensor data structure for inter-task communication
 */
struct SensorData {
    float temperature;
    float humidity;
    uint8_t aqi;
    uint16_t tvoc;
    uint16_t eco2;
    bool aht21Valid;
    bool ens160Valid;
    uint32_t timestamp;
    bool isNewData;  // Indicates if data hasn't been sent yet
    
    SensorData() : temperature(0.0f), humidity(0.0f), aqi(0), tvoc(0), eco2(0),
                   aht21Valid(false), ens160Valid(false), timestamp(0), isNewData(false) {}
};

/**
 * @brief Sensor health status for heartbeat monitoring
 */
struct SensorHealthStatus {
    bool isHealthy;
    uint8_t consecutiveFailures;
    uint32_t lastCheckTime;
    uint32_t expectedId;
    uint32_t actualId;
    std::string sensorName;
    
    SensorHealthStatus() : isHealthy(true), consecutiveFailures(0), lastCheckTime(0), 
                          expectedId(0), actualId(0), sensorName("") {}
};

/**
 * @brief RTOS Manager for multi-task environmental monitoring system
 * 
 * Manages FreeRTOS tasks for sensor reading, network transmission,
 * system monitoring, and watchdog protection.
 */
class RTOSManager {
private:
    static constexpr const char* TAG = "RTOSManager";
    
    // Task priorities
    static constexpr UBaseType_t SENSOR_TASK_PRIORITY = 5;
    static constexpr UBaseType_t NETWORK_TASK_PRIORITY = 4;
    static constexpr UBaseType_t MONITOR_TASK_PRIORITY = 6;  
    static constexpr UBaseType_t WATCHDOG_TASK_PRIORITY = 8;
    static constexpr UBaseType_t HEARTBEAT_TASK_PRIORITY = 7;  // High priority for health monitoring
    
    // Task stack sizes
    static constexpr uint32_t SENSOR_TASK_STACK_SIZE = 4096;
    static constexpr uint32_t NETWORK_TASK_STACK_SIZE = 8192;
    static constexpr uint32_t MONITOR_TASK_STACK_SIZE = 8192;
    static constexpr uint32_t WATCHDOG_TASK_STACK_SIZE = 2048;
    static constexpr uint32_t HEARTBEAT_TASK_STACK_SIZE = 4096;
    
    // Queue sizes
    static constexpr uint32_t SENSOR_DATA_QUEUE_SIZE = 10;
    static constexpr uint32_t NETWORK_CMD_QUEUE_SIZE = 5;
    
    // Heartbeat configuration
    static constexpr uint8_t MAX_CONSECUTIVE_FAILURES = 5;
    static constexpr uint32_t HEARTBEAT_CHECK_TIMEOUT_MS = 5000;  // 5 second timeout for ID checks
    
    // Task handles
    TaskHandle_t sensorTaskHandle_;
    TaskHandle_t networkTaskHandle_;
    TaskHandle_t monitorTaskHandle_;
    TaskHandle_t watchdogTaskHandle_;
    TaskHandle_t heartbeatTaskHandle_;
    
    // Inter-task communication
    QueueHandle_t sensorDataQueue_;
    QueueHandle_t networkCmdQueue_;
    EventGroupHandle_t systemEventGroup_;
    SemaphoreHandle_t sensorMutex_;
    
    // System state
    std::atomic<bool> systemRunning_;
    std::atomic<uint32_t> sensorReadingsCount_;
    std::atomic<uint32_t> networkTransmissionsCount_;
    std::atomic<uint32_t> networkErrorsCount_;
    
    // Sensor objects
    std::shared_ptr<I2CManager> i2cManager_;
    std::shared_ptr<AHT21Sensor> aht21Sensor_;
    std::shared_ptr<ENS160Sensor> ens160Sensor_;
    
    // Configuration manager
    std::unique_ptr<ConfigManager> configManager_;
    
    // Configuration
    uint32_t sensorReadingIntervalMs_;
    uint32_t networkTransmissionIntervalMs_;
    uint32_t monitorReportIntervalMs_;
    bool intervalsAreSame_;  // Track if read and post intervals are the same
    
    // Configurable parameters
    uint32_t heartbeatIntervalSeconds_;
    uint32_t sensorQueueSize_;
    bool detailedLoggingEnabled_;
    bool sleepModeEnabled_;
    uint32_t networkMaxRetries_;
    uint32_t networkRetryDelayMs_;
    uint32_t networkTimeoutMs_;
    std::string serverUrl_;
    int i2cSdaPin_;
    int i2cSclPin_;
    
    // Air quality validation thresholds
    uint8_t aqiMin_;
    uint8_t aqiMax_;
    uint16_t tvocMin_;
    uint16_t tvocMax_;
    uint16_t eco2Min_;
    uint16_t eco2Max_;
    
    // Heartbeat monitoring
    SensorHealthStatus aht21Health_;
    SensorHealthStatus ens160Health_;
    std::atomic<bool> heartbeatTaskRunning_;
    
    // Health monitoring
    std::shared_ptr<HealthMonitor> healthMonitor_;
    std::unique_ptr<HealthServer> healthServer_;
    
    // Expected sensor IDs
    static constexpr uint16_t AHT21_EXPECTED_STATUS_MASK = 0x08;  // Calibrated bit
    static constexpr uint16_t ENS160_EXPECTED_PART_ID = 0x0160;
    
    // Static task functions
    static void sensorTask(void* parameter);
    static void networkTask(void* parameter);
    static void monitorTask(void* parameter);
    static void watchdogTask(void* parameter);
    static void heartbeatTask(void* parameter);
    
    // Helper functions
    void initialiseWatchdog();
    void feedWatchdog();
    void handleSystemError(const char* errorMsg);
    bool loadConfiguration(const char* configPath);
    
    /**
     * @brief Check sensor health by verifying its ID
     * @param healthStatus Reference to sensor health status structure
     * @param idCheckFunction Function to get sensor ID
     * @return true if sensor is healthy, false otherwise
     */
    template<typename F>
    bool checkSensorHealth(SensorHealthStatus& healthStatus, F idCheckFunction);

public:
    RTOSManager();
    ~RTOSManager();
    
    /**
     * @brief Initialise the RTOS manager and create all tasks
     */
    esp_err_t initialise(std::shared_ptr<I2CManager> i2cManager,
                        std::shared_ptr<AHT21Sensor> aht21Sensor,
                        std::shared_ptr<ENS160Sensor> ens160Sensor,
                        const char* configPath);
    
    /**
     * @brief Start all tasks and begin system operation
     */
    esp_err_t start();
    
    /**
     * @brief Stop all tasks and clean up resources
     */
    void stop();
    
    /**
     * @brief Check if system is running
     */
    bool isRunning() const { return systemRunning_.load(); }
    
    /**
     * @brief Get system statistics
     */
    void getStatistics(uint32_t& readingsCount, uint32_t& transmissionsCount, uint32_t& errorsCount) const;
    
    /**
     * @brief Validate configuration before starting system
     * @return true if configuration is valid, false otherwise
     */
    bool validateConfiguration() const;
    
    // Disable copy constructor and assignment operator
    RTOSManager(const RTOSManager&) = delete;
    RTOSManager& operator=(const RTOSManager&) = delete;
    
    // Enable move constructor and assignment operator
    RTOSManager(RTOSManager&& other) noexcept;
    RTOSManager& operator=(RTOSManager&& other) noexcept;

    // Method to set health monitor from outside
    void setHealthMonitor(std::shared_ptr<HealthMonitor> healthMonitor) {
        healthMonitor_ = healthMonitor;
    }
}; 