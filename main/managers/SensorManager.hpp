#pragma once

#include "sensors/ISensor.hpp"
#include "I2CManager.hpp"
#include "esp_log.h"
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <cstring>

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
 * 
 * Uses fixed-size arrays instead of std::string to avoid memory corruption
 * when passed through FreeRTOS queues
 */
struct UnifiedSensorData {
    static constexpr size_t MAX_FIELDS = 10;
    static constexpr size_t MAX_SENSORS = 5;
    static constexpr size_t MAX_FIELD_NAME_LEN = 16;
    static constexpr size_t MAX_SENSOR_NAME_LEN = 16;
    
    struct FieldValue {
        char name[MAX_FIELD_NAME_LEN];
        float value;
        bool used;
        
        FieldValue() : value(0.0f), used(false) {
            name[0] = '\0';
        }
    };
    
    struct SensorValidity {
        char name[MAX_SENSOR_NAME_LEN];
        bool valid;
        bool used;
        
        SensorValidity() : valid(false), used(false) {
            name[0] = '\0';
        }
    };
    
    FieldValue values[MAX_FIELDS];
    SensorValidity sensors[MAX_SENSORS];
    uint32_t timestamp;
    bool isNewData;
    
    UnifiedSensorData() : timestamp(0), isNewData(false) {
        // Initialize arrays
        for (auto& field : values) {
            field = FieldValue();
        }
        for (auto& sensor : sensors) {
            sensor = SensorValidity();
        }
    }
    
    /**
     * @brief Get a specific value from the data
     * @param field Field name (e.g., "temperature", "humidity", "pressure")
     * @return Value if found, 0.0f otherwise
     */
    float getValue(const char* field) const {
        for (const auto& fv : values) {
            if (fv.used && strcmp(fv.name, field) == 0) {
                return fv.value;
            }
        }
        return 0.0f;
    }
    
    /**
     * @brief Check if a field exists in the data
     * @param field Field name
     * @return true if field exists, false otherwise
     */
    bool hasValue(const char* field) const {
        for (const auto& fv : values) {
            if (fv.used && strcmp(fv.name, field) == 0) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Check if a sensor is valid
     * @param sensorName Sensor name
     * @return true if sensor is valid, false otherwise
     */
    bool isSensorValid(const char* sensorName) const {
        for (const auto& sv : sensors) {
            if (sv.used && strcmp(sv.name, sensorName) == 0) {
                return sv.valid;
            }
        }
        return false;
    }
    
    /**
     * @brief Add a field value
     * @param field Field name
     * @param value Field value
     * @return true if added successfully, false if no space
     */
    bool addValue(const char* field, float value) {
        for (auto& fv : values) {
            if (!fv.used) {
                strncpy(fv.name, field, MAX_FIELD_NAME_LEN - 1);
                fv.name[MAX_FIELD_NAME_LEN - 1] = '\0';
                fv.value = value;
                fv.used = true;
                return true;
            }
        }
        return false; // No space available
    }
    
    /**
     * @brief Add a sensor validity
     * @param sensorName Sensor name
     * @param valid Whether sensor is valid
     * @return true if added successfully, false if no space
     */
    bool addSensor(const char* sensorName, bool valid) {
        for (auto& sv : sensors) {
            if (!sv.used) {
                strncpy(sv.name, sensorName, MAX_SENSOR_NAME_LEN - 1);
                sv.name[MAX_SENSOR_NAME_LEN - 1] = '\0';
                sv.valid = valid;
                sv.used = true;
                return true;
            }
        }
        return false; // No space available
    }
    
    /**
     * @brief Clear all data
     */
    void clear() {
        for (auto& field : values) {
            field = FieldValue();
        }
        for (auto& sensor : sensors) {
            sensor = SensorValidity();
        }
        timestamp = 0;
        isNewData = false;
    }
    
    /**
     * @brief Get count of valid sensors
     * @return Number of valid sensors
     */
    size_t getValidSensorCount() const {
        size_t count = 0;
        for (const auto& sv : sensors) {
            if (sv.used && sv.valid) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * @brief Get count of fields
     * @return Number of fields
     */
    size_t getFieldCount() const {
        size_t count = 0;
        for (const auto& fv : values) {
            if (fv.used) {
                count++;
            }
        }
        return count;
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