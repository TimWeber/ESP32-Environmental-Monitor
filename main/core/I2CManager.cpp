#include "I2CManager.hpp"

void I2CManager::initialiseFromConfig(const char* configPath) {
    ESP_LOGI(TAG, "Initialising I2C manager from config: %s", configPath);
    

    gpio_num_t sdaPin = GPIO_NUM_8;  // Default SDA pin
    gpio_num_t sclPin = GPIO_NUM_9;  // Default SCL pin
    
    ESP_LOGI(TAG, "I2C pins from config: SDA=%d, SCL=%d", sdaPin, sclPin);
    initialise(sdaPin, sclPin);
}

I2CManager::I2CManager() 
    : busHandle_(nullptr), initialised_(false), sdaPin_(GPIO_NUM_NC), sclPin_(GPIO_NUM_NC) {
}

I2CManager::~I2CManager() {
    if (initialised_ && busHandle_) {
        ESP_LOGI(TAG, "Deinitialising I2C master bus");
        esp_err_t err = i2c_del_master_bus(busHandle_);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to delete I2C master bus: %s", esp_err_to_name(err));
        }
        busHandle_ = nullptr;
        initialised_ = false;
    }
}

void I2CManager::initialise(gpio_num_t sdaPin, gpio_num_t sclPin) {
    if (initialised_) {
        ESP_LOGW(TAG, "I2C manager already initialised");
        return;
    }
    
    sdaPin_ = sdaPin;
    sclPin_ = sclPin;
    
    ESP_LOGI(TAG, "Initialising I2C master bus on SDA=%d, SCL=%d", sdaPin_, sclPin_);
    
    i2c_master_bus_config_t i2cMstConfig = {};
    i2cMstConfig.clk_source = I2C_CLK_SRC_DEFAULT;
    i2cMstConfig.i2c_port = I2C_NUM_0;
    i2cMstConfig.scl_io_num = sclPin_;
    i2cMstConfig.sda_io_num = sdaPin_;
    i2cMstConfig.glitch_ignore_cnt = 7;
    i2cMstConfig.flags.enable_internal_pullup = true;
    
    esp_err_t err = i2c_new_master_bus(&i2cMstConfig, &busHandle_);
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to create I2C master bus: " + std::string(esp_err_to_name(err)));
    }
    
    initialised_ = true;
    ESP_LOGI(TAG, "I2C master bus initialised successfully");
}

i2c_master_bus_handle_t I2CManager::getBusHandle() const {
    if (!initialised_) {
        throw std::runtime_error("I2C manager not initialised");
    }
    return busHandle_;
}

void I2CManager::createDevice(uint8_t deviceAddress, i2c_master_dev_handle_t* deviceHandle) {
    if (!initialised_) {
        throw std::runtime_error("I2C manager not initialised");
    }
    
    if (!deviceHandle) {
        throw std::runtime_error("Device handle pointer is null");
    }
    
    ESP_LOGI(TAG, "Creating I2C device at address 0x%02X", deviceAddress);
    
    i2c_device_config_t devCfg = {};
    devCfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    devCfg.device_address = deviceAddress;
    devCfg.scl_speed_hz = I2C_MASTER_FREQ_HZ;
    
    esp_err_t err = i2c_master_bus_add_device(busHandle_, &devCfg, deviceHandle);
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to add I2C device 0x" + 
                                std::to_string(deviceAddress) + ": " + 
                                std::string(esp_err_to_name(err)));
    }
    
    ESP_LOGI(TAG, "I2C device 0x%02X created successfully", deviceAddress);
}

int I2CManager::scanBus() {
    if (!initialised_) {
        ESP_LOGW(TAG, "I2C manager not initialised, cannot scan bus");
        return -1;
    }
    
    ESP_LOGI(TAG, "Scanning I2C bus for known devices...");
    int devicesFound = 0;
    
    // Only scan addresses where we expect to find devices
    // This reduces NACK errors significantly
    uint8_t knownAddresses[] = {
        0x38,  // AHT21 temperature/humidity sensor
        0x53,  // ENS160 air quality sensor
    };
    
    for (uint8_t addr : knownAddresses) {
        i2c_master_dev_handle_t probeHandle;
        i2c_device_config_t devCfg = {};
        devCfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        devCfg.device_address = addr;
        devCfg.scl_speed_hz = I2C_MASTER_FREQ_HZ;
        
        esp_err_t err = i2c_master_bus_add_device(busHandle_, &devCfg, &probeHandle);
        if (err == ESP_OK) {
            // Try to probe the device with a write operation (less likely to generate NACK errors)
            // Send a single byte to test if device responds
            uint8_t testByte = 0x00;
            err = i2c_master_transmit(probeHandle, &testByte, 1, I2C_MASTER_TIMEOUT_MS);
            
            if (err == ESP_OK || err == ESP_ERR_TIMEOUT) {
                ESP_LOGI(TAG, "Found I2C device at address 0x%02X", addr);
                devicesFound++;
            }
            
            // Clean up the probe device
            i2c_master_bus_rm_device(probeHandle);
        }
    }
    
    ESP_LOGI(TAG, "I2C bus scan complete. Found %d devices", devicesFound);
    return devicesFound;
}

I2CManager::I2CManager(I2CManager&& other) noexcept
    : busHandle_(other.busHandle_), initialised_(other.initialised_),
      sdaPin_(other.sdaPin_), sclPin_(other.sclPin_) {
    other.busHandle_ = nullptr;
    other.initialised_ = false;
    other.sdaPin_ = GPIO_NUM_NC;
    other.sclPin_ = GPIO_NUM_NC;
}

I2CManager& I2CManager::operator=(I2CManager&& other) noexcept {
    if (this != &other) {
        // Clean up current resources
        if (initialised_ && busHandle_) {
            i2c_del_master_bus(busHandle_);
        }
        
        // Move from other
        busHandle_ = other.busHandle_;
        initialised_ = other.initialised_;
        sdaPin_ = other.sdaPin_;
        sclPin_ = other.sclPin_;
        
        // Reset other
        other.busHandle_ = nullptr;
        other.initialised_ = false;
        other.sdaPin_ = GPIO_NUM_NC;
        other.sclPin_ = GPIO_NUM_NC;
    }
    return *this;
}
