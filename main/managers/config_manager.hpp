#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include "core/calibration.hpp"

/**
 * @brief Abstract configuration interface
 * 
 * This interface allows for protocol-agnostic configuration loading.
 * Different implementations can handle JSON, XML, YAML, binary, etc.
 */
class IConfigProvider {
public:
    virtual ~IConfigProvider() = default;
    
    // Generic value retrieval methods
    virtual bool getString(const std::string& key, std::string& value) const = 0;
    virtual bool getInt(const std::string& key, int& value) const = 0;
    virtual bool getUInt(const std::string& key, uint32_t& value) const = 0;
    virtual bool getBool(const std::string& key, bool& value) const = 0;
    virtual bool getFloat(const std::string& key, float& value) const = 0;
    
    // Section-based access (for nested structures like "sensors", "network", etc.)
    virtual std::unique_ptr<IConfigProvider> getSection(const std::string& section) const = 0;
    
    // Check if a key exists
    virtual bool hasKey(const std::string& key) const = 0;
    
    // Check if a section exists
    virtual bool hasSection(const std::string& section) const = 0;
};

/**
 * @brief Configuration manager that uses a config provider
 * 
 * This class is protocol-agnostic and works with any IConfigProvider implementation.
 */
class ConfigManager {
public:
    explicit ConfigManager(std::unique_ptr<IConfigProvider> provider);
    ~ConfigManager() = default;
    
    // Configuration loading methods
    bool loadDataCollectionConfig();
    bool loadDataTransmissionConfig();
    bool loadConnectivityConfig();
    bool loadSystemMonitoringConfig();
    
    // Configuration validation
    bool validateConfiguration() const;
    
    // Configuration access methods
    uint32_t getSensorReadingIntervalMs() const { return sensorReadingIntervalMs_; }
    uint32_t getNetworkTransmissionIntervalMs() const { return networkTransmissionIntervalMs_; }
    uint32_t getMonitorReportIntervalMs() const { return monitorReportIntervalMs_; }
    uint32_t getHeartbeatIntervalSeconds() const { return heartbeatIntervalSeconds_; }
    uint32_t getSensorQueueSize() const { return sensorQueueSize_; }
    bool isDetailedLoggingEnabled() const { return detailedLoggingEnabled_; }
    bool isSleepModeEnabled() const { return sleepModeEnabled_; }
    uint32_t getNetworkMaxRetries() const { return networkMaxRetries_; }
    uint32_t getNetworkRetryDelayMs() const { return networkRetryDelayMs_; }
    uint32_t getNetworkTimeoutMs() const { return networkTimeoutMs_; }
    const std::string& getServerUrl() const { return serverUrl_; }
    
    // I2C configuration
    int getI2CSdaPin() const { return i2cSdaPin_; }
    int getI2CSclPin() const { return i2cSclPin_; }
    
    // Air quality validation thresholds
    uint8_t getAqiMin() const { return aqiMin_; }
    uint8_t getAqiMax() const { return aqiMax_; }
    uint16_t getTvocMin() const { return tvocMin_; }
    uint16_t getTvocMax() const { return tvocMax_; }
    uint16_t getEco2Min() const { return eco2Min_; }
    uint16_t getEco2Max() const { return eco2Max_; }
    
    // Calibration access
    const CalibrationManager* getCalibrationManager() const { return calibrationManager_.get(); }
    
private:
    std::unique_ptr<IConfigProvider> configProvider_;
    
    // Configuration values
    uint32_t sensorReadingIntervalMs_;
    uint32_t networkTransmissionIntervalMs_;
    uint32_t monitorReportIntervalMs_;
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
    
    // Calibration manager
    std::unique_ptr<CalibrationManager> calibrationManager_;
    
    // Default values
    static constexpr uint32_t DEFAULT_SENSOR_INTERVAL_MS = 5000;
    static constexpr uint32_t DEFAULT_NETWORK_INTERVAL_MS = 30000;
    static constexpr uint32_t DEFAULT_MONITOR_INTERVAL_MS = 60000;
    static constexpr uint32_t DEFAULT_HEARTBEAT_SECONDS = 60;
    static constexpr uint32_t DEFAULT_SENSOR_QUEUE_SIZE = 10;
    static constexpr uint32_t DEFAULT_NETWORK_MAX_RETRIES = 3;
    static constexpr uint32_t DEFAULT_NETWORK_RETRY_DELAY_MS = 1000;
    static constexpr uint32_t DEFAULT_NETWORK_TIMEOUT_MS = 10000;
    static constexpr int DEFAULT_I2C_SDA_PIN = 8;
    static constexpr int DEFAULT_I2C_SCL_PIN = 9;
    static constexpr uint8_t DEFAULT_AQI_MIN = 0;
    static constexpr uint8_t DEFAULT_AQI_MAX = 5;
    static constexpr uint16_t DEFAULT_TVOC_MIN = 0;
    static constexpr uint16_t DEFAULT_TVOC_MAX = 10000;
    static constexpr uint16_t DEFAULT_ECO2_MIN = 0;
    static constexpr uint16_t DEFAULT_ECO2_MAX = 10000;
    

};

/**
 * @brief Factory function to create appropriate config provider based on file extension
 */
std::unique_ptr<IConfigProvider> createConfigProvider(const std::string& configPath); 