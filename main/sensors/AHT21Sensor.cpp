#include "AHT21Sensor.hpp"
#include "cJSON.h"
#include <cstring>

AHT21Sensor::AHT21Sensor(I2CManager& i2cManager)
    : i2cManager_(i2cManager), deviceHandle_(nullptr), initialised_(false),
      temperatureOffset_(0.0f), humidityOffset_(0.0f) {
}

AHT21Sensor::~AHT21Sensor() {
    if (deviceHandle_) {
        ESP_LOGI(TAG, "Removing AHT21 device");
        esp_err_t err = i2c_master_bus_rm_device(deviceHandle_);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to remove AHT21 device: %s", esp_err_to_name(err));
        }
        deviceHandle_ = nullptr;
    }
    initialised_ = false;
}

void AHT21Sensor::initialise() {
    ESP_LOGI(TAG, "Initialising AHT21 sensor...");
    
    // Create I2C device handle
    i2cManager_.createDevice(AHT21_ADDRESS, &deviceHandle_);
    
    // Initial delay
    vTaskDelay(pdMS_TO_TICKS(AHT21_INIT_DELAY_MS));
    
    // First, check if device is present by trying to read status
    uint8_t status;
    esp_err_t err = i2c_master_receive(deviceHandle_, &status, 1, 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AHT21 device not responding at address 0x%02X: %s", 
                 AHT21_ADDRESS, esp_err_to_name(err));
        ESP_LOGW(TAG, "AHT21 sensor not found - continuing without sensor");
        
        // Clean up device handle
        if (deviceHandle_) {
            esp_err_t rm_err = i2c_master_bus_rm_device(deviceHandle_);
            if (rm_err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to remove AHT21 device: %s", esp_err_to_name(rm_err));
            }
            deviceHandle_ = nullptr;
        }
        
        // Don't throw exception - just mark as not initialised
        initialised_ = false;
        return;
    }
        
        ESP_LOGI(TAG, "AHT21 device found at address 0x%02X", AHT21_ADDRESS);
        
        // Reset sensor first
        reset();
        vTaskDelay(pdMS_TO_TICKS(AHT21_RESET_DELAY_MS));
        
        // Check if sensor is calibrated
        status = getStatus();
        ESP_LOGI(TAG, "AHT21 initial status: 0x%02X", status);
        
        if (!(status & AHT21_STATUS_CALIBRATED)) {
            ESP_LOGI(TAG, "AHT21 not calibrated, performing initialisation...");
            
            // Send initialisation command
            uint8_t initData[2] = {0x08, 0x00};
            writeCommand(AHT21_CMD_INITIALIZE, initData, 2);
            vTaskDelay(pdMS_TO_TICKS(AHT21_INIT_DELAY_MS));
            
            // Check calibration status again
            status = getStatus();
            ESP_LOGI(TAG, "AHT21 status after initialisation: 0x%02X", status);
            
                    if (!(status & AHT21_STATUS_CALIBRATED)) {
            ESP_LOGW(TAG, "AHT21 failed to calibrate - continuing without sensor");
            
            // Clean up device handle
            if (deviceHandle_) {
                esp_err_t rm_err = i2c_master_bus_rm_device(deviceHandle_);
                if (rm_err != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to remove AHT21 device: %s", esp_err_to_name(rm_err));
                }
                deviceHandle_ = nullptr;
            }
            
            // Don't throw exception - just mark as not initialised
            initialised_ = false;
            return;
        }
    }
    
    initialised_ = true;
    ESP_LOGI(TAG, "AHT21 initialised successfully");
}

void AHT21Sensor::initialiseFromConfig(const char* configPath) {
    ESP_LOGI(TAG, "Initialising AHT21 sensor from config: %s", configPath);
    
    initialise();
    
    if (!initialised_) {
        ESP_LOGW(TAG, "AHT21 sensor initialisation failed - system will continue without this sensor");
    }
}

AHT21Data AHT21Sensor::readData() {
    AHT21Data data;
    
    if (!initialised_) {
        throw std::runtime_error("AHT21 sensor not initialised");
    }
    
    try {
        // Trigger measurement
        uint8_t triggerData[2] = {0x33, 0x00};
        writeCommand(AHT21_CMD_TRIGGER, triggerData, 2);
        
        // Wait for measurement to complete
        if (!waitForReady(1000)) {
            ESP_LOGW(TAG, "AHT21 measurement timeout");
            return data; // returns invalid data
        }
        
        uint8_t rawData[6];
        readData(rawData, 6);
        
        // Check if measurement is valid
        if (rawData[0] & AHT21_STATUS_BUSY) {
            ESP_LOGW(TAG, "AHT21 still busy after measurement");
            return data; // returns invalid data
        }
        
        // Convert raw data to temperature and humidity
        uint32_t humidityRaw = ((uint32_t)rawData[1] << 12) | 
                               ((uint32_t)rawData[2] << 4) | 
                               (rawData[3] >> 4);
        
        uint32_t temperatureRaw = ((uint32_t)(rawData[3] & 0x0F) << 16) | 
                                  ((uint32_t)rawData[4] << 8) | 
                                  rawData[5];
        
        // Convert to actual values and apply offsets
        data.humidity = (float)humidityRaw * 100.0f / 1048576.0f + humidityOffset_;
        data.temperature = (float)temperatureRaw * 200.0f / 1048576.0f - 50.0f + temperatureOffset_;
        data.valid = true;
        
        ESP_LOGD(TAG, "AHT21 reading: T=%.2f°C, H=%.2f%% (with offsets)", 
                 data.temperature, data.humidity);
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Failed to read AHT21 data: %s", e.what());
        data.valid = false;
    }
    
    return data;
}

void AHT21Sensor::reset() {
    if (!deviceHandle_) {
        throw std::runtime_error("AHT21 device not initialized");
    }
    
    ESP_LOGI(TAG, "Resetting AHT21 sensor");
    
    try {
        // AHT21 soft reset - send reset command without data
        writeCommand(AHT21_CMD_RESET);
        vTaskDelay(pdMS_TO_TICKS(AHT21_RESET_DELAY_MS));
    } catch (const std::exception& e) {
        ESP_LOGW(TAG, "AHT21 reset failed: %s", e.what());
    }
    
    initialised_ = false;
}

bool AHT21Sensor::isReady() {
    if (!initialised_) {
        return false;
    }
    
    try {
        uint8_t status = getStatus();
        return !(status & AHT21_STATUS_BUSY) && (status & AHT21_STATUS_CALIBRATED);
    } catch (const std::exception&) {
        return false;
    }
}

uint8_t AHT21Sensor::getStatus() {
    if (!deviceHandle_) {
        throw std::runtime_error("AHT21 device not initialized");
    }
    
    uint8_t status;
    esp_err_t err = i2c_master_receive(deviceHandle_, &status, 1, 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read AHT21 status: %s", esp_err_to_name(err));
        throw std::runtime_error("Failed to read AHT21 status: " + std::string(esp_err_to_name(err)));
    }
    
    return status;
}

void AHT21Sensor::writeCommand(uint8_t cmd, const uint8_t* data, size_t dataLen) {
    if (!deviceHandle_) {
        throw std::runtime_error("AHT21 device not initialized");
    }
    
    uint8_t buffer[8]; // Maximum command size
    buffer[0] = cmd;
    
    size_t totalLen = 1;
    if (data && dataLen > 0) {
        if (dataLen > 7) {
            throw std::runtime_error("AHT21 command data too long");
        }
        memcpy(&buffer[1], data, dataLen);
        totalLen += dataLen;
    }
    
    esp_err_t err = i2c_master_transmit(deviceHandle_, buffer, totalLen, 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AHT21 write command failed: %s (cmd=0x%02X)", 
                 esp_err_to_name(err), cmd);
        throw std::runtime_error("Failed to write AHT21 command: " + std::string(esp_err_to_name(err)));
    }
}

void AHT21Sensor::readData(uint8_t* buffer, size_t len) {
    if (!deviceHandle_) {
        throw std::runtime_error("AHT21 device not initialized");
    }
    
    if (!buffer) {
        throw std::runtime_error("AHT21 read buffer is null");
    }
    
    esp_err_t err = i2c_master_receive(deviceHandle_, buffer, len, 1000);
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to read AHT21 data: " + std::string(esp_err_to_name(err)));
    }
}

bool AHT21Sensor::waitForReady(uint32_t timeoutMs) {
    uint32_t startTime = xTaskGetTickCount();
    uint32_t timeoutTicks = pdMS_TO_TICKS(timeoutMs);
    
    while ((xTaskGetTickCount() - startTime) < timeoutTicks) {
        try {
            uint8_t status = getStatus();
            if (!(status & AHT21_STATUS_BUSY)) {
                return true;
            }
        } catch (const std::exception&) {
            // Continue trying on communication errors
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
    
    return false;
}

AHT21Sensor::AHT21Sensor(AHT21Sensor&& other) noexcept
    : i2cManager_(other.i2cManager_), deviceHandle_(other.deviceHandle_), 
      initialised_(other.initialised_) {
    other.deviceHandle_ = nullptr;
    other.initialised_ = false;
}

AHT21Sensor& AHT21Sensor::operator=(AHT21Sensor&& other) noexcept {
    if (this != &other) {
        // Clean up current resources
        if (deviceHandle_) {
            i2c_master_bus_rm_device(deviceHandle_);
        }
        
        // Move from other
        deviceHandle_ = other.deviceHandle_;
        initialised_ = other.initialised_;
        
        // Reset other
        other.deviceHandle_ = nullptr;
        other.initialised_ = false;
    }
    return *this;
}
