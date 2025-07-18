#include "ENS160Sensor.hpp"
#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

ENS160Sensor::ENS160Sensor(I2CManager& i2cManager) 
    : i2cManager_(i2cManager), deviceHandle_(nullptr), initialised_(false), startupTime_(0),
      aqiMin_(0), aqiMax_(5), tvocMin_(0), tvocMax_(10000), eco2Min_(0), eco2Max_(10000) {
}

ENS160Sensor::~ENS160Sensor() {
    if (deviceHandle_) {
        ESP_LOGI(TAG, "Removing ENS160 device");
        esp_err_t err = i2c_master_bus_rm_device(deviceHandle_);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to remove ENS160 device: %s", esp_err_to_name(err));
        }
        deviceHandle_ = nullptr;
    }
    initialised_ = false;
}

ENS160Sensor::ENS160Sensor(ENS160Sensor&& other) noexcept 
    : i2cManager_(other.i2cManager_), deviceHandle_(other.deviceHandle_), 
      initialised_(other.initialised_), startupTime_(other.startupTime_) {
    other.deviceHandle_ = nullptr;
    other.initialised_ = false;
}

ENS160Sensor& ENS160Sensor::operator=(ENS160Sensor&& other) noexcept {
    if (this != &other) {
        if (deviceHandle_) {
            ESP_LOGI(TAG, "Removing ENS160 device in move assignment");
            esp_err_t err = i2c_master_bus_rm_device(deviceHandle_);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to remove ENS160 device: %s", esp_err_to_name(err));
            }
        }
        deviceHandle_ = other.deviceHandle_;
        initialised_ = other.initialised_;
        startupTime_ = other.startupTime_;
        other.deviceHandle_ = nullptr;
        other.initialised_ = false;
    }
    return *this;
}

void ENS160Sensor::initialise() {
    ESP_LOGI(TAG, "Initialising ENS160 sensor...");
    
    if (!i2cManager_.isInitialised()) {
        throw std::runtime_error("I2C manager not initialised");
    }
    
    // Create I2C device
    i2cManager_.createDevice(ENS160_ADDRESS, &deviceHandle_);
    
    // Initial delay
    vTaskDelay(pdMS_TO_TICKS(ENS160_STARTUP_DELAY_MS));
    
    try {
        // Verify part ID
        uint16_t partId = getPartId();
        ESP_LOGI(TAG, "ENS160 Part ID: 0x%04X", partId);
        if (partId != ENS160_PART_ID) {
            throw std::runtime_error("Invalid ENS160 Part ID");
        }
        
        // Reset the sensor first to ensure clean state
        ESP_LOGI(TAG, "Resetting ENS160 sensor...");
        writeRegister(ENS160_REG_OPMODE, ENS160_OPMODE_SLEEP);
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // Put sensor in idle mode first
        setOperatingMode(ENS160_OPMODE_IDLE);
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // Clear general purpose read registers
        writeRegister(ENS160_REG_COMMAND, ENS160_COMMAND_CLRGPR);
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // Set to standard operating mode
        setOperatingMode(ENS160_OPMODE_STANDARD);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Check initial status
        uint8_t initialStatus = getDeviceStatus();
        ESP_LOGI(TAG, "ENS160 initial status after mode set: 0x%02X", initialStatus);
        
        // Don't wait for full warmup during initialisation - let it warm up naturally
        ESP_LOGI(TAG, "ENS160 initialisation complete, sensor will warm up during operation");
        
        initialised_ = true;
        startupTime_ = xTaskGetTickCount();
        ESP_LOGI(TAG, "ENS160 initialised successfully - warmup will continue in background");
        
    } catch (...) {
        if (deviceHandle_) {
            ESP_LOGI(TAG, "Removing ENS160 device due to initialisation failure");
            esp_err_t err = i2c_master_bus_rm_device(deviceHandle_);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to remove ENS160 device: %s", esp_err_to_name(err));
            }
            deviceHandle_ = nullptr;
        }
        throw;
    }
}

void ENS160Sensor::initialiseFromConfig(const char* configPath) {
    ESP_LOGI(TAG, "Initialising ENS160 sensor from config: %s", configPath);
    

    initialise();
}

void ENS160Sensor::setValidationThresholds(uint8_t aqiMin, uint8_t aqiMax, 
                                          uint16_t tvocMin, uint16_t tvocMax,
                                          uint16_t eco2Min, uint16_t eco2Max) {
    aqiMin_ = aqiMin;
    aqiMax_ = aqiMax;
    tvocMin_ = tvocMin;
    tvocMax_ = tvocMax;
    eco2Min_ = eco2Min;
    eco2Max_ = eco2Max;
    
    ESP_LOGI(TAG, "ENS160 validation thresholds set: AQI[%d-%d], TVOC[%d-%d], eCO2[%d-%d]", 
              aqiMin_, aqiMax_, tvocMin_, tvocMax_, eco2Min_, eco2Max_);
}

ENS160Data ENS160Sensor::readData() {
    if (!initialised_) {
        throw std::runtime_error("ENS160 not initialised");
    }
    
    ENS160Data data;
    
    try {
        // Calculate time since startup
        TickType_t currentTime = xTaskGetTickCount();
        uint32_t warmupTimeMs = pdTICKS_TO_MS(currentTime - startupTime_);
        
        // Check device status
        uint8_t deviceStatus = getDeviceStatus();
        ESP_LOGI(TAG, "ENS160 device status: 0x%02X (warmup time: %lu ms)", deviceStatus, warmupTimeMs);
        
        // Debug: Read all data registers to see what's actually there
        uint8_t rawAqi = readRegister(ENS160_REG_DATA_AQI);
        uint16_t rawTvoc = readRegister16(ENS160_REG_DATA_TVOC);
        uint16_t rawEco2 = readRegister16(ENS160_REG_DATA_ECO2);
        uint8_t dataStatus = readRegister(ENS160_REG_DATA_STATUS);
        ESP_LOGI(TAG, "ENS160 raw registers: AQI=0x%02X(%d), TVOC=0x%04X(%d), eCO2=0x%04X(%d), DataStatus=0x%02X", 
                rawAqi, rawAqi, rawTvoc, rawTvoc, rawEco2, rawEco2, dataStatus);
        
        // If we're getting all zeros for multiple readings, try a full reset
        static uint32_t lastResetTime = 0;
        static uint32_t zeroReadingCount = 0;
        uint32_t currentResetTime = pdTICKS_TO_MS(currentTime);
        
        // Check if this reading is all zeros
        // Pre-read the data to check for zeros
        uint8_t tempAqi = readRegister(ENS160_REG_DATA_AQI);
        uint16_t tempTvoc = readRegister16(ENS160_REG_DATA_TVOC);
        uint16_t tempEco2 = readRegister16(ENS160_REG_DATA_ECO2);
        
        if (tempAqi == 0 && tempTvoc == 0 && tempEco2 == 0) {
            zeroReadingCount++;
        } else {
            zeroReadingCount = 0; // Reset counter on good reading
        }
        
        // If we have 3+ consecutive zero readings and it's been >2 minutes since last reset
        if (zeroReadingCount >= 3 && (currentResetTime - lastResetTime) > 120000) {
            ESP_LOGW(TAG, "ENS160 giving zeros for %lu readings, performing full reset", zeroReadingCount);
            
            // Full reset sequence
            writeRegister(ENS160_REG_OPMODE, ENS160_OPMODE_SLEEP);
            vTaskDelay(pdMS_TO_TICKS(100));
            
            writeRegister(ENS160_REG_COMMAND, ENS160_COMMAND_CLRGPR);
            vTaskDelay(pdMS_TO_TICKS(100));
            
            setOperatingMode(ENS160_OPMODE_IDLE);
            vTaskDelay(pdMS_TO_TICKS(100));
            
            setOperatingMode(ENS160_OPMODE_STANDARD);
            vTaskDelay(pdMS_TO_TICKS(100));
            
            lastResetTime = currentResetTime;
            zeroReadingCount = 0;
            startupTime_ = currentTime; // Reset startup time
            
            ESP_LOGI(TAG, "ENS160 reset completed, restarting measurements");
        }
        
        uint8_t statusBits = deviceStatus & 0x0F;
        
        // Use the pre-read values from the reset check above
        data.aqi = tempAqi;
        data.tvoc = tempTvoc;
        data.eco2 = tempEco2;
        data.status = deviceStatus;
        
        ESP_LOGI(TAG, "ENS160 raw readings: AQI=%d, TVOC=%d, eCO2=%d, status=0x%02X", 
                data.aqi, data.tvoc, data.eco2, data.status);
        
        // Validate data quality using configurable thresholds
        bool hasValidReadings = (data.aqi >= aqiMin_ && data.aqi <= aqiMax_) && 
                               (data.tvoc >= tvocMin_ && data.tvoc <= tvocMax_) && 
                               (data.eco2 >= eco2Min_ && data.eco2 <= eco2Max_);
        
        ESP_LOGI(TAG, "ENS160 validation: AQI[%d-%d]=%d, TVOC[%d-%d]=%d, eCO2[%d-%d]=%d, valid=%s", 
                aqiMin_, aqiMax_, data.aqi, tvocMin_, tvocMax_, data.tvoc, 
                eco2Min_, eco2Max_, data.eco2, hasValidReadings ? "YES" : "NO");
        
        // Check if sensor is in a normal state
        if (statusBits == ENS160_DEVICE_STATUS_NORMAL) {
            // Check data status for new data
            uint8_t dataStatus = getDataStatus();
            ESP_LOGI(TAG, "ENS160 data status: 0x%02X", dataStatus);
            
            // Accept data in normal state regardless of NEWDAT flag during early operation
            // The sensor might not set NEWDAT consistently in all conditions
            if (hasValidReadings) {
                data.valid = true;
                ESP_LOGI(TAG, "ENS160 normal operation - valid data: AQI=%d, TVOC=%d, eCO2=%d", 
                        data.aqi, data.tvoc, data.eco2);
            } else {
                ESP_LOGW(TAG, "ENS160 normal operation but invalid readings - rejecting: AQI=%d, TVOC=%d, eCO2=%d", 
                        data.aqi, data.tvoc, data.eco2);
                data.valid = false;
            }
        } else if (statusBits == ENS160_DEVICE_STATUS_WARM_UP) {
            ESP_LOGI(TAG, "ENS160 warming up (status=0x%02X), warmup time: %lu ms", deviceStatus, warmupTimeMs);
            // During warmup, accept data after 30 seconds (reduced from 60 seconds)
            if (warmupTimeMs > 30000 && hasValidReadings) {
                data.valid = true;
                ESP_LOGI(TAG, "ENS160 warmup >30s with valid data: AQI=%d, TVOC=%d, eCO2=%d", 
                        data.aqi, data.tvoc, data.eco2);
            } else {
                data.valid = false;
            }
        } else if (statusBits == ENS160_DEVICE_STATUS_INITIAL_START) {
            ESP_LOGI(TAG, "ENS160 initial startup phase (status=0x%02X), warmup time: %lu ms", deviceStatus, warmupTimeMs);
            // Accept data during initial start if it's been more than 15 seconds
            if (warmupTimeMs > 15000 && hasValidReadings) {
                data.valid = true;
                ESP_LOGI(TAG, "ENS160 initial start >15s with valid data: AQI=%d, TVOC=%d, eCO2=%d", 
                        data.aqi, data.tvoc, data.eco2);
            } else {
                data.valid = false;
            }
        } else if (statusBits == ENS160_DEVICE_STATUS_INVALID_OUTPUT) {
            ESP_LOGW(TAG, "ENS160 invalid output (status=0x%02X), warmup time: %lu ms", deviceStatus, warmupTimeMs);
            // With invalid output status, accept data if values are reasonable
            if (hasValidReadings) {
                data.valid = true;
                ESP_LOGI(TAG, "ENS160 invalid status but valid data - using it: AQI=%d, TVOC=%d, eCO2=%d", 
                        data.aqi, data.tvoc, data.eco2);
            } else {
                ESP_LOGW(TAG, "ENS160 invalid status and invalid data - rejecting: AQI=%d, TVOC=%d, eCO2=%d", 
                        data.aqi, data.tvoc, data.eco2);
                data.valid = false;
            }
        } else {
            ESP_LOGW(TAG, "ENS160 unknown status (0x%02X), warmup time: %lu ms", deviceStatus, warmupTimeMs);
            // For unknown status, accept if data looks reasonable
            if (hasValidReadings) {
                data.valid = true;
                ESP_LOGI(TAG, "ENS160 unknown status but valid data - using it: AQI=%d, TVOC=%d, eCO2=%d", 
                        data.aqi, data.tvoc, data.eco2);
            } else {
                ESP_LOGW(TAG, "ENS160 unknown status and invalid data - rejecting: AQI=%d, TVOC=%d, eCO2=%d", 
                        data.aqi, data.tvoc, data.eco2);
                data.valid = false;
            }
        }
        
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Failed to read ENS160 data: %s", e.what());
        throw;
    }
    
    return data;
}

void ENS160Sensor::setOperatingMode(uint8_t mode) {
    writeRegister(ENS160_REG_OPMODE, mode);
    ESP_LOGD(TAG, "ENS160 operating mode set to 0x%02X", mode);
}

void ENS160Sensor::setEnvironmentalCompensation(float temperature, float humidity) {
    if (!initialised_) {
        ESP_LOGW(TAG, "ENS160 not initialised, skipping environmental compensation");
        return;
    }
    
    try {
        // Convert temperature to ENS160 format (Kelvin * 64)
        uint16_t tempData = (uint16_t)((temperature + 273.15f) * 64.0f);
        
        // Convert humidity to ENS160 format (RH% * 512)
        uint16_t rhData = (uint16_t)(humidity * 512.0f);
        
        writeRegister16(ENS160_REG_TEMP_IN, tempData);
        writeRegister16(ENS160_REG_RH_IN, rhData);
        
        ESP_LOGD(TAG, "ENS160 environmental compensation set: T=%.1f°C, RH=%.1f%% (raw: 0x%04X, 0x%04X)", 
                temperature, humidity, tempData, rhData);
    } catch (const std::exception& e) {
        ESP_LOGE(TAG, "Failed to set ENS160 environmental compensation: %s", e.what());
    }
}

bool ENS160Sensor::isReady() {
    if (!initialised_) {
        return false;
    }
    
    try {
        uint8_t deviceStatus = getDeviceStatus();
        return (deviceStatus & 0x0F) == ENS160_DEVICE_STATUS_NORMAL;
    } catch (...) {
        return false;
    }
}

uint8_t ENS160Sensor::getDeviceStatus() {
    return readRegister(ENS160_REG_DEVICE_STATUS);
}

uint8_t ENS160Sensor::getDataStatus() {
    return readRegister(ENS160_REG_DATA_STATUS);
}

bool ENS160Sensor::isNewDataAvailable() {
    try {
        uint8_t dataStatus = getDataStatus();
        return (dataStatus & ENS160_DATA_STATUS_NEWDAT) != 0;
    } catch (...) {
        return false;
    }
}

uint16_t ENS160Sensor::getPartId() {
    return readRegister16(ENS160_REG_PART_ID);
}

void ENS160Sensor::writeRegister(uint8_t reg, const uint8_t* data, size_t len) {
    if (!deviceHandle_) {
        throw std::runtime_error("ENS160 device not initialized");
    }
    
    uint8_t buffer[16]; // Maximum write size
    if (len + 1 > sizeof(buffer)) {
        throw std::runtime_error("ENS160 write data too long");
    }
    
    buffer[0] = reg;
    if (data && len > 0) {
        memcpy(&buffer[1], data, len);
    }
    
    esp_err_t err = i2c_master_transmit(deviceHandle_, buffer, len + 1, 1000);
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to write ENS160 register 0x" + 
                                std::to_string(reg) + ": " + 
                                std::string(esp_err_to_name(err)));
    }
}

void ENS160Sensor::writeRegister(uint8_t reg, uint8_t value) {
    writeRegister(reg, &value, 1);
}

void ENS160Sensor::writeRegister16(uint8_t reg, uint16_t value) {
    uint8_t data[2];
    data[0] = value & 0xFF;        // LSB first
    data[1] = (value >> 8) & 0xFF; // MSB second
    writeRegister(reg, data, 2);
}

void ENS160Sensor::readRegister(uint8_t reg, uint8_t* buffer, size_t len) {
    if (!deviceHandle_) {
        throw std::runtime_error("ENS160 device not initialized");
    }
    
    if (!buffer) {
        throw std::runtime_error("ENS160 read buffer is null");
    }
    
    esp_err_t err = i2c_master_transmit_receive(deviceHandle_, &reg, 1, buffer, len, 1000);
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to read ENS160 register 0x" + 
                                std::to_string(reg) + ": " + 
                                std::string(esp_err_to_name(err)));
    }
}

uint8_t ENS160Sensor::readRegister(uint8_t reg) {
    uint8_t value;
    readRegister(reg, &value, 1);
    return value;
}

uint16_t ENS160Sensor::readRegister16(uint8_t reg) {
    uint8_t data[2];
    readRegister(reg, data, 2);
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8); // Little-endian
}

bool ENS160Sensor::waitForWarmup(uint32_t timeoutMs) {
    TickType_t startTime = xTaskGetTickCount();
    TickType_t timeoutTicks = pdMS_TO_TICKS(timeoutMs);
    
    while ((xTaskGetTickCount() - startTime) < timeoutTicks) {
        try {
            uint8_t deviceStatus = getDeviceStatus();
            uint8_t statusBits = deviceStatus & 0x0F;
            
            ESP_LOGD(TAG, "ENS160 warmup status: 0x%02X", deviceStatus);
            
            if (statusBits == ENS160_DEVICE_STATUS_NORMAL) {
                ESP_LOGI(TAG, "ENS160 warmup complete");
                return true;
            }
            
            if (statusBits == ENS160_DEVICE_STATUS_INVALID_OUTPUT) {
                ESP_LOGW(TAG, "ENS160 invalid output during warmup");
            }
            
        } catch (const std::exception& e) {
            ESP_LOGW(TAG, "ENS160 warmup check failed: %s", e.what());
        }
        
        vTaskDelay(pdMS_TO_TICKS(500)); // Check every 500ms
    }
    
    ESP_LOGW(TAG, "ENS160 warmup timeout after %lu ms", timeoutMs);
    return false;
}