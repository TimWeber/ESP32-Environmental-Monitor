#pragma once

#include "sensors/ISensor.hpp"
#include "I2CManager.hpp"
#include "esp_log.h"
#include <memory>
#include <map>
#include <vector>
#include <string>

/**
 * @brief Sensor health status for monitoring
 */
struct SensorHealthStatus {
    bool isHealthy;
    uint8_t consecutiveFailures;
    uint32_t lastCheckTime;
    uint32_t expectedId;
    uint32_t actualId;
    std::string sensorName;
    std::string status;
    
    SensorHealthStatus() : isHealthy(true), consecutiveFailures(0), lastCheckTime(0), 
                          expectedId(0), actualId(0), sensorName(""), status("unknown") {}
};

/**
 * @brief Sensor configuration structure
 */
struct SensorConfig {
    std::string name;
    std::string type;
    bool enabled;
    uint8_t address;
    uint32_t intervalMs;
    std::map<std::string, float> thresholds;
    
    SensorConfig() : enabled(true), address(0), intervalMs(5000) {}
};

/**
 * @brief Unified sensor data structure
 */
struct UnifiedSensorData {
    std::map<std::string, float> values;  // Field name -> value mapping
    std::map<std::string, bool> valid;    // Sensor name -> validity mapping
    uint32_t timestamp;
    bool isNewData;
    
    UnifiedSensorData() : timestamp(0), isNewData(false) {}
    
    /**
     * @brief Get a specific value from the data
     * @param field Field name (e.g., "temperature", "humidity", "pressure")
     * @return Value if found, 0.0f otherwise
     */
    float getValue(const std::string& field) const {
        auto it = values.find(field);
        return (it != values.end()) ? it->second : 0.0f;
    }
    
    /**
     * @brief Check if a field exists in the data
     * @param field Field name
     * @return true if field exists, false otherwise
     */
    bool hasValue(const std::string& field) const {
        return values.find(field) != values.end();
    }
    
    /**
     * @brief Check if a sensor is valid
     * @param sensorName Sensor name
     * @return true if sensor is valid, false otherwise
     */
    bool isSensorValid(const std::string& sensorName) const {
        auto it = valid.find(sensorName);
        return (it != valid.end()) ? it->second : false;
    }
    
    /**
     * @brief Get all valid sensors
     * @return Vector of valid sensor names
     */
    std::vector<std::string> getValidSensors() const {
        std::vector<std::string> validSensors;
        for (const auto& [name, isValid] : valid) {
            if (isValid) {
                validSensors.push_back(name);
            }
        }
        return validSensors;
    }
};

/**
 * @brief Sensor Manager for unified sensor management
 * 
 * Manages multiple sensors through a common interface, providing:
 * - Unified sensor initialization
 * - Centralized error handling
 * - Health monitoring
 * - Configuration-driven sensor management
 */
class SensorManager {
private:
    static constexpr const char* TAG = "SensorManager";
    
    // Sensor storage
    std::map<std::string, std::shared_ptr<ISensor>> sensors_;
    std::map<std::string, SensorHealthStatus> sensorHealth_;
    std::map<std::string, SensorConfig> sensorConfigs_;
    
    // I2C Manager reference
    std::shared_ptr<I2CManager> i2cManager_;
    
    // Configuration
    const char* configPath_;
    bool initialised_;
    
    // Statistics
    uint32_t totalReadings_;
    uint32_t failedReadings_;
    uint32_t lastHealthCheck_;
    
    // Helper methods
    bool loadSensorConfiguration();
    bool validateSensorConfiguration(const SensorConfig& config);
    void updateSensorHealth(const std::string& sensorName, bool success);
    std::vector<std::string> getEnabledSensors() const;

public:
    /**
     * @brief Constructor
     * @param i2cManager Reference to I2C manager
     */
    explicit SensorManager(std::shared_ptr<I2CManager> i2cManager);
    
    /**
     * @brief Destructor
     */
    ~SensorManager() = default;
    
    /**
     * @brief Initialize the sensor manager
     * @return true if initialization successful, false otherwise
     */
    bool initialise();
    
    /**
     * @brief Initialize all sensors from configuration
     * @param configPath Path to configuration file
     * @return true if initialization successful, false otherwise
     */
    bool initialiseFromConfig(const char* configPath);
    
    /**
     * @brief Add a sensor to the manager
     * @param name Sensor name
     * @param sensor Sensor instance
     * @return true if added successfully, false otherwise
     */
    bool addSensor(const std::string& name, std::shared_ptr<ISensor> sensor);
    
    /**
     * @brief Remove a sensor from the manager
     * @param name Sensor name
     * @return true if removed successfully, false otherwise
     */
    bool removeSensor(const std::string& name);
    
    /**
     * @brief Enable a sensor
     * @param name Sensor name
     * @return true if enabled successfully, false otherwise
     */
    bool enableSensor(const std::string& name);
    
    /**
     * @brief Disable a sensor
     * @param name Sensor name
     * @return true if disabled successfully, false otherwise
     */
    bool disableSensor(const std::string& name);
    
    /**
     * @brief Read data from all sensors
     * @return Unified sensor data
     */
    UnifiedSensorData readAllSensors();
    
    /**
     * @brief Read data from a specific sensor
     * @param name Sensor name
     * @return Sensor reading
     */
    SensorReading readSensor(const std::string& name);
    
    /**
     * @brief Check health of all sensors
     */
    void checkAllSensorHealth();
    
    /**
     * @brief Get sensor health status
     * @param name Sensor name
     * @return Health status
     */
    SensorHealthStatus getSensorHealth(const std::string& name) const;
    
    /**
     * @brief Get all sensor health statuses
     * @return Map of sensor health statuses
     */
    std::map<std::string, SensorHealthStatus> getAllSensorHealth() const;
    
    /**
     * @brief Get list of all sensors
     * @return Vector of sensor names
     */
    std::vector<std::string> getAllSensors() const;
    
    /**
     * @brief Get list of active sensors
     * @return Vector of active sensor names
     */
    std::vector<std::string> getActiveSensors() const;
    
    /**
     * @brief Get number of active sensors
     * @return Number of active sensors
     */
    size_t getActiveSensorCount() const;
    
    /**
     * @brief Check if a sensor exists
     * @param name Sensor name
     * @return true if sensor exists, false otherwise
     */
    bool hasSensor(const std::string& name) const;
    
    /**
     * @brief Check if a sensor is active
     * @param name Sensor name
     * @return true if sensor is active, false otherwise
     */
    bool isSensorActive(const std::string& name) const;
    
    /**
     * @brief Get sensor configuration
     * @param name Sensor name
     * @return Sensor configuration
     */
    SensorConfig getSensorConfig(const std::string& name) const;
    
    /**
     * @brief Get a specific sensor by name
     * @param name Sensor name
     * @return Shared pointer to sensor, or nullptr if not found
     */
    std::shared_ptr<ISensor> getSensor(const std::string& name) const {
        auto it = sensors_.find(name);
        return (it != sensors_.end()) ? it->second : nullptr;
    }
    
    /**
     * @brief Update sensor configuration
     * @param name Sensor name
     * @param config New configuration
     * @return true if updated successfully, false otherwise
     */
    bool updateSensorConfig(const std::string& name, const SensorConfig& config);
    
    /**
     * @brief Get statistics
     * @param totalReadings Total readings count
     * @param failedReadings Failed readings count
     */
    void getStatistics(uint32_t& totalReadings, uint32_t& failedReadings) const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Get I2C manager reference
     * @return I2C manager reference
     */
    std::shared_ptr<I2CManager> getI2CManager() const { return i2cManager_; }
}; 