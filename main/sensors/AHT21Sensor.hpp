#pragma once

#include "managers/I2CManager.hpp"
#include "sensors/ISensor.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdexcept>

/**
 * @brief Structure to hold AHT21 sensor data
 */
struct AHT21Data {
    float temperature;  // °C
    float humidity;     // %RH
    bool valid;         // Data validity flag
    
    AHT21Data() : temperature(0.0f), humidity(0.0f), valid(false) {}
};

/**
 * @brief Modern C++ driver for AHT21 temperature and humidity sensor
 * 
 * Uses new ESP-IDF i2c_master API with RAII for proper resource management.
 * Provides exception-safe interface with automatic device cleanup.
 */
class AHT21Sensor : public ISensor {
private:
    static constexpr const char* TAG = "AHT21";
    static constexpr uint8_t AHT21_ADDRESS = 0x38;
    static constexpr uint8_t AHT21_CMD_INITIALIZE = 0xBE;
    static constexpr uint8_t AHT21_CMD_TRIGGER = 0xAC;
    static constexpr uint8_t AHT21_CMD_RESET = 0xBA;
    static constexpr uint8_t AHT21_STATUS_BUSY = 0x80;
    static constexpr uint8_t AHT21_STATUS_CALIBRATED = 0x08;
    static constexpr uint32_t AHT21_MEASUREMENT_DELAY_MS = 80;
    static constexpr uint32_t AHT21_RESET_DELAY_MS = 20;
    static constexpr uint32_t AHT21_INIT_DELAY_MS = 40;
    
    I2CManager& i2cManager_;
    i2c_master_dev_handle_t deviceHandle_;
    bool initialised_;
    float temperatureOffset_;
    float humidityOffset_;

public:
    /**
     * @brief Constructor
     * @param i2cManager Reference to I2C manager instance
     */
    explicit AHT21Sensor(I2CManager& i2cManager);
    
    /**
     * @brief Destructor - automatically cleans up device resources
     */
    ~AHT21Sensor();
    

    
    /**
     * @brief Get sensor status byte
     * @return Status byte from sensor
     * @throws std::runtime_error if communication fails
     */
    uint8_t getStatus();



    // ISensor interface implementations
    bool initialise() override;
    bool initialiseFromConfig(const char* configPath) override;
    SensorReading readData() override;
    bool isInitialised() const override { return initialised_; }
    bool isReady() override;
    std::string getSensorType() const override { return "AHT21"; }
    uint8_t getAddress() const override { return AHT21_ADDRESS; }
    bool reset() override;
    std::string getStatus() const override;

    // Disable copy constructor and assignment operator
    AHT21Sensor(const AHT21Sensor&) = delete;
    AHT21Sensor& operator=(const AHT21Sensor&) = delete;
    
    // Enable move constructor and assignment operator
    AHT21Sensor(AHT21Sensor&& other) noexcept;
    AHT21Sensor& operator=(AHT21Sensor&& other) noexcept;

private:
    /**
     * @brief Write command to sensor
     * @param cmd Command byte
     * @param data Additional data bytes (optional)
     * @param dataLen Length of additional data
     * @throws std::runtime_error if write fails
     */
    void writeCommand(uint8_t cmd, const uint8_t* data = nullptr, size_t dataLen = 0);
    
    /**
     * @brief Read data from sensor
     * @param buffer Buffer to store read data
     * @param len Number of bytes to read
     * @throws std::runtime_error if read fails
     */
    void readData(uint8_t* buffer, size_t len);
    
    /**
     * @brief Wait for sensor to complete measurement
     * @param timeoutMs Maximum time to wait in milliseconds
     * @return true if sensor is ready, false if timeout
     */
    bool waitForReady(uint32_t timeoutMs = 1000);
};
