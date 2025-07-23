#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdexcept>

/**
 * @brief Modern C++ I2C Manager using new ESP-IDF i2c_master API
 * 
 * This class manages I2C bus initialization and provides a centralized
 * interface for all I2C devices. Uses RAII for proper resource management.
 */
class I2CManager {
private:
    static constexpr const char* TAG = "I2CManager";
    static constexpr uint32_t I2C_MASTER_FREQ_HZ = 100000; // 100kHz
    static constexpr uint32_t I2C_MASTER_TIMEOUT_MS = 1000;
    
    i2c_master_bus_handle_t busHandle_;
    bool initialised_;
    gpio_num_t sdaPin_;
    gpio_num_t sclPin_;

public:
    /**
     * @brief Constructor
     */
    I2CManager();
    
    /**
     * @brief Destructor - automatically cleans up I2C resources
     */
    ~I2CManager();
    
    /**
     * @brief Initialise I2C master bus
     * @param sdaPin SDA GPIO pin number
     * @param sclPin SCL GPIO pin number
     * @throws std::runtime_error if initialisation fails
     */
    void initialise(gpio_num_t sdaPin, gpio_num_t sclPin);
    /**
     * @brief Initialise I2C manager with configuration from file
     * @param configPath Path to configuration file
     */
    void initialiseFromConfig(const char* configPath);
    
    /**
     * @brief Get the I2C bus handle for device creation
     * @return i2c_master_bus_handle_t Bus handle
     * @throws std::runtime_error if not initialised
     */
    i2c_master_bus_handle_t getBusHandle() const;
    
    /**
     * @brief Check if I2C manager is initialised
     * @return true if initialised, false otherwise
     */
    bool isInitialised() const { return initialised_; }
    
    /**
     * @brief Create device handle for a specific I2C address
     * @param deviceAddress 7-bit I2C device address
     * @param deviceHandle Output parameter for device handle
     * @throws std::runtime_error if device creation fails
     */
    void createDevice(uint8_t deviceAddress, i2c_master_dev_handle_t* deviceHandle);
    
    /**
     * @brief Scan I2C bus for devices
     * @return Number of devices found
     */
    int scanBus();

    // Disable copy constructor and assignment operator
    I2CManager(const I2CManager&) = delete;
    I2CManager& operator=(const I2CManager&) = delete;
    
    // Enable move constructor and assignment operator
    I2CManager(I2CManager&& other) noexcept;
    I2CManager& operator=(I2CManager&& other) noexcept;
};
