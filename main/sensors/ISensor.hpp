#pragma once

#include <string>
#include <map>
#include <cstdint>

/**
 * @brief Generic sensor reading structure
 */
struct SensorReading {
    std::map<std::string, float> values;  // Field name -> value mapping
    bool valid;                           // Overall data validity
    uint32_t timestamp;                   // Reading timestamp
    
    SensorReading() : valid(false), timestamp(0) {}
    
    /**
     * @brief Get a specific value from the reading
     * @param field Field name (e.g., "temperature", "humidity", "pressure")
     * @return Value if found, 0.0f otherwise
     */
    float getValue(const std::string& field) const {
        auto it = values.find(field);
        return (it != values.end()) ? it->second : 0.0f;
    }
    
    /**
     * @brief Set a value in the reading
     * @param field Field name
     * @param value Value to set
     */
    void setValue(const std::string& field, float value) {
        values[field] = value;
    }
    
    /**
     * @brief Check if a field exists in the reading
     * @param field Field name
     * @return true if field exists, false otherwise
     */
    bool hasValue(const std::string& field) const {
        return values.find(field) != values.end();
    }
};

/**
 * @brief Abstract sensor interface
 * 
 * All sensors must implement this interface to work with the SensorManager.
 * This provides a common interface for different sensor types.
 */
class ISensor {
public:
    virtual ~ISensor() = default;
    
    /**
     * @brief Initialize the sensor
     * @return true if initialization successful, false otherwise
     */
    virtual bool initialise() = 0;
    
    /**
     * @brief Initialize the sensor with configuration
     * @param configPath Path to configuration file
     * @return true if initialization successful, false otherwise
     */
    virtual bool initialiseFromConfig(const char* configPath) = 0;
    
    /**
     * @brief Read data from the sensor
     * @return SensorReading with sensor data
     */
    virtual SensorReading readData() = 0;
    
    /**
     * @brief Check if sensor is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool isInitialised() const = 0;
    
    /**
     * @brief Check if sensor is ready for reading
     * @return true if ready, false otherwise
     */
    virtual bool isReady() = 0;
    
    /**
     * @brief Get sensor type identifier
     * @return Sensor type string (e.g., "AHT21", "ENS160", "BMP280")
     */
    virtual std::string getSensorType() const = 0;
    
    /**
     * @brief Get sensor address (for I2C sensors)
     * @return Sensor I2C address
     */
    virtual uint8_t getAddress() const = 0;
    
    /**
     * @brief Reset the sensor
     * @return true if reset successful, false otherwise
     */
    virtual bool reset() = 0;
    
    /**
     * @brief Get sensor status information
     * @return Status string describing sensor state
     */
    virtual std::string getStatus() const = 0;
}; 