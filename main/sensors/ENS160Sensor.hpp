#pragma once

#include "managers/I2CManager.hpp"
#include "sensors/ISensor.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdexcept>

/**
 * @brief ENS160 sensor data structure
 */
struct ENS160Data {
    uint16_t aqi;           // Air Quality Index (1-5)
    uint16_t tvoc;          // Total VOC in ppb
    uint16_t eco2;          // Equivalent CO2 in ppm
    float temperature;      // °C
    float humidity;         // %RH
    uint8_t status;         // Sensor status
    bool valid;             // Data validity flag
    
    ENS160Data() : aqi(0), tvoc(0), eco2(0), temperature(0.0f), humidity(0.0f), status(0), valid(false) {}
};

/**
 * @brief Modern C++ driver for ENS160 digital multi-gas sensor
 * 
 * Uses ESP-IDF i2c_master API with RAII for proper resource management.
 * Provides exception-safe interface with automatic device cleanup.
 */
class ENS160Sensor : public ISensor {
private:
    static constexpr const char* TAG = "ENS160";
    static constexpr uint8_t ENS160_ADDRESS = 0x53;
    
    // Register addresses
    static constexpr uint8_t ENS160_REG_PART_ID = 0x00;
    static constexpr uint8_t ENS160_REG_OPMODE = 0x10;
    static constexpr uint8_t ENS160_REG_CONFIG = 0x11;
    static constexpr uint8_t ENS160_REG_COMMAND = 0x12;
    static constexpr uint8_t ENS160_REG_TEMP_IN = 0x13;
    static constexpr uint8_t ENS160_REG_RH_IN = 0x15;
    static constexpr uint8_t ENS160_REG_DATA_STATUS = 0x20;
    static constexpr uint8_t ENS160_REG_DATA_AQI = 0x21;
    static constexpr uint8_t ENS160_REG_DATA_TVOC = 0x22;
    static constexpr uint8_t ENS160_REG_DATA_ECO2 = 0x24;
    static constexpr uint8_t ENS160_REG_DATA_T = 0x30;
    static constexpr uint8_t ENS160_REG_DATA_RH = 0x32;
    static constexpr uint8_t ENS160_REG_DEVICE_STATUS = 0x38;
    static constexpr uint8_t ENS160_REG_ERROR = 0x39;
    
    // Operating modes
    static constexpr uint8_t ENS160_OPMODE_SLEEP = 0x00;
    static constexpr uint8_t ENS160_OPMODE_IDLE = 0x01;
    static constexpr uint8_t ENS160_OPMODE_STANDARD = 0x02;
    static constexpr uint8_t ENS160_OPMODE_LP = 0x03;
    
    // Commands
    static constexpr uint8_t ENS160_COMMAND_NOP = 0x00;
    static constexpr uint8_t ENS160_COMMAND_GET_APPVER = 0x0E;
    static constexpr uint8_t ENS160_COMMAND_CLRGPR = 0xCC;
    
    // Status bits
    static constexpr uint8_t ENS160_DATA_STATUS_NEWDAT = 0x02;
    static constexpr uint8_t ENS160_DATA_STATUS_NEWGPR = 0x01;
    
    // Device status bits
    static constexpr uint8_t ENS160_DEVICE_STATUS_NORMAL = 0x00;
    static constexpr uint8_t ENS160_DEVICE_STATUS_WARM_UP = 0x01;
    static constexpr uint8_t ENS160_DEVICE_STATUS_INITIAL_START = 0x02;
    static constexpr uint8_t ENS160_DEVICE_STATUS_INVALID_OUTPUT = 0x03;
    
    // Timing constants
    static constexpr uint32_t ENS160_STARTUP_DELAY_MS = 100;
    static constexpr uint32_t ENS160_WARMUP_TIME_MS = 3000;
    static constexpr uint32_t ENS160_STANDARD_MEASUREMENT_TIME_MS = 1000;
    
    // Expected Part ID
    static constexpr uint16_t ENS160_PART_ID = 0x0160;
    
    I2CManager& i2cManager_;
    i2c_master_dev_handle_t deviceHandle_;
    bool initialised_;
    uint32_t startupTime_;
    
    // Configurable validation thresholds
    uint8_t aqiMin_;
    uint8_t aqiMax_;
    uint16_t tvocMin_;
    uint16_t tvocMax_;
    uint16_t eco2Min_;
    uint16_t eco2Max_;
    
    // Configurable warmup timeout
    uint32_t warmupTimeoutMs_;

public:
    /**
     * @brief Constructor
     * @param i2cManager Reference to I2C manager instance
     */
    explicit ENS160Sensor(I2CManager& i2cManager);
    
    /**
     * @brief Destructor - automatically cleans up device resources
     */
    ~ENS160Sensor();
    
    /**
     * @brief Read air quality data from sensor (internal method)
     * @return ENS160Data structure with AQI, TVOC, eCO2, and validity flag
     * @throws std::runtime_error if reading fails
     */
    ENS160Data readDataInternal();
    
    /**
     * @brief Set operating mode (internal method)
     * @param mode Operating mode (SLEEP, IDLE, STANDARD, LP)
     * @throws std::runtime_error if setting mode fails
     */
    void setOperatingMode(uint8_t mode);
    
    /**
     * @brief Set environmental compensation data (internal method)
     * @param temperature Temperature in °C
     * @param humidity Relative humidity in %RH
     * @throws std::runtime_error if setting compensation fails
     */
    void setEnvironmentalCompensation(float temperature, float humidity);
    
    /**
     * @brief Check if sensor is ready for measurement (internal method)
     * @return true if ready, false otherwise
     */
    bool isReadyInternal();
    
    /**
     * @brief Set validation thresholds for air quality data (internal method)
     * @param aqiMin Minimum AQI value (default: 1)
     * @param aqiMax Maximum AQI value (default: 5)
     * @param tvocMin Minimum TVOC value in ppb (default: 1)
     * @param tvocMax Maximum TVOC value in ppb (default: 10000)
     * @param eco2Min Minimum eCO2 value in ppm (default: 200)
     * @param eco2Max Maximum eCO2 value in ppm (default: 10000)
     */
    void setValidationThresholds(uint8_t aqiMin, uint8_t aqiMax, 
                                uint16_t tvocMin, uint16_t tvocMax,
                                uint16_t eco2Min, uint16_t eco2Max);
    
    /**
     * @brief Set warmup timeout for error state reset (internal method)
     * @param timeoutMs Timeout in milliseconds
     */
    void setWarmupTimeout(uint32_t timeoutMs);
    
    /**
     * @brief Initialise the ENS160 sensor (internal method)
     * @throws std::runtime_error if initialisation fails
     */
    void initialiseInternal();
    
    /**
     * @brief Initialise the ENS160 sensor with config loading (internal method)
     * @param config_path Path to the configuration file
     * @throws std::runtime_error if initialisation fails
     */
    void initialiseFromConfigInternal(const char* config_path);
    
    /**
     * @brief Get sensor device status
     * @return Device status byte
     * @throws std::runtime_error if communication fails
     */
    uint8_t getDeviceStatus();
    
    /**
     * @brief Get data status
     * @return Data status byte
     * @throws std::runtime_error if communication fails
     */
    uint8_t getDataStatus();
    
    /**
     * @brief Check if new data is available
     * @return true if new data available, false otherwise
     */
    bool isNewDataAvailable();
    
    /**
     * @brief Get Part ID for verification
     * @return Part ID (should be 0x0160)
     * @throws std::runtime_error if communication fails
     */
    uint16_t getPartId();

    // ISensor interface implementations
    bool initialise() override;
    bool initialiseFromConfig(const char* configPath) override;
    SensorReading readData() override;
    bool isInitialised() const override { return initialised_; }
    bool isReady() override;
    std::string getSensorType() const override { return "ENS160"; }
    uint8_t getAddress() const override { return ENS160_ADDRESS; }
    bool reset() override;
    std::string getStatus() const override;

    // Disable copy constructor and assignment operator
    ENS160Sensor(const ENS160Sensor&) = delete;
    ENS160Sensor& operator=(const ENS160Sensor&) = delete;
    
    // Enable move constructor and assignment operator
    ENS160Sensor(ENS160Sensor&& other) noexcept;
    ENS160Sensor& operator=(ENS160Sensor&& other) noexcept;

private:
    /**
     * @brief Write to a register
     * @param reg Register address
     * @param data Data to write
     * @param len Length of data
     * @throws std::runtime_error if write fails
     */
    void writeRegister(uint8_t reg, const uint8_t* data, size_t len);
    
    /**
     * @brief Write single byte to register
     * @param reg Register address
     * @param value Value to write
     * @throws std::runtime_error if write fails
     */
    void writeRegister(uint8_t reg, uint8_t value);
    
    /**
     * @brief Write 16-bit value to register
     * @param reg Register address
     * @param value 16-bit value to write (little-endian)
     * @throws std::runtime_error if write fails
     */
    void writeRegister16(uint8_t reg, uint16_t value);
    
    /**
     * @brief Read from a register
     * @param reg Register address
     * @param buffer Buffer to store read data
     * @param len Number of bytes to read
     * @throws std::runtime_error if read fails
     */
    bool readRegister(uint8_t reg, uint8_t* buffer, size_t len);
    
    /**
     * @brief Read single byte from register
     * @param reg Register address
     * @return Register value
     * @throws std::runtime_error if read fails
     */
    uint8_t readRegister(uint8_t reg);
    
    /**
     * @brief Read 16-bit value from register
     * @param reg Register address
     * @return 16-bit register value (little-endian)
     * @throws std::runtime_error if read fails
     */
    uint16_t readRegister16(uint8_t reg);
    
    /**
     * @brief Wait for sensor warmup
     * @param timeoutMs Maximum time to wait in milliseconds
     * @return true if warmup complete, false if timeout
     */
    bool waitForWarmup(uint32_t timeoutMs = 10000);
};
