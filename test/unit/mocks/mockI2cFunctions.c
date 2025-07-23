/*
 * Mock I2C Functions Implementation for ESP32 Environmental Monitor Tests
 * 
 * This file provides mock implementations of I2C functions for unit testing.
 * These mocks simulate I2C hardware responses without requiring real sensors.
 */

#include "mockI2cFunctions.h"
#include "esp_log.h"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "MOCK_I2C";

// Mock response data for different sensors
static uint8_t mockAht21Response[] = {0xAC, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t mockEns160PartId[] = {0x16, 0x00}; // ENS160 Part ID
static uint8_t mockEns160Data[] = {0x50, 0x01, 0x00, 0x64, 0x00}; // AQI=80, TVOC=256, eCO2=100

// Mock device addresses
#define MOCK_AHT21_ADDRESS 0x38
#define MOCK_ENS160_ADDRESS 0x53

// Mock device status
static bool mockAht21Connected = true;
static bool mockEns160Connected = true;

// Mock initialization and cleanup
void mockI2cFunctionsInit(void) {
    ESP_LOGI(TAG, "Initializing mock I2C functions");
    mockAht21Connected = true;
    mockEns160Connected = true;
}

void mockI2cFunctionsCleanup(void) {
    ESP_LOGI(TAG, "Cleaning up mock I2C functions");
    mockAht21Connected = false;
    mockEns160Connected = false;
}

// Mock I2C master initialisation
esp_err_t mockI2cMasterInit(const i2c_config_t* config) {
    ESP_LOGI(TAG, "Mock I2C master initialisation called");
    
    if (config == NULL) {
        ESP_LOGE(TAG, "Mock I2C config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Mock I2C initialised with SDA=%d, SCL=%d, freq=%d", 
              config->sda_io_num, config->scl_io_num, config->master.clk_speed);
    
    return ESP_OK;
}

// Mock I2C master device creation
i2c_master_bus_handle_t mockI2cMasterBusAddDevice(i2c_master_bus_handle_t busHandle, uint8_t deviceAddress) {
    ESP_LOGI(TAG, "Mock I2C device creation for address 0x%02X", deviceAddress);
    
    if (busHandle == NULL) {
        ESP_LOGE(TAG, "Mock I2C bus handle is NULL");
        return NULL;
    }
    
    // Return a mock device handle (just a pointer to the address)
    static uint8_t mockDeviceHandle;
    mockDeviceHandle = deviceAddress;
    
    ESP_LOGI(TAG, "Mock I2C device created successfully");
    return (i2c_master_bus_handle_t)&mockDeviceHandle;
}

// Mock I2C master device removal
esp_err_t mockI2cMasterBusRemoveDevice(i2c_master_bus_handle_t busHandle, i2c_master_dev_handle_t deviceHandle) {
    ESP_LOGI(TAG, "Mock I2C device removal called");
    
    if (busHandle == NULL || deviceHandle == NULL) {
        ESP_LOGE(TAG, "Mock I2C device removal with NULL handles");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Mock I2C device removed successfully");
    return ESP_OK;
}

// Mock I2C master transmit
esp_err_t mockI2cMasterTransmit(i2c_master_dev_handle_t deviceHandle, const uint8_t* writeBuffer, size_t writeSize, uint32_t timeoutMs) {
    ESP_LOGI(TAG, "Mock I2C transmit called with %zu bytes", writeSize);
    
    if (deviceHandle == NULL || writeBuffer == NULL) {
        ESP_LOGE(TAG, "Mock I2C transmit with NULL parameters");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Simulate successful transmission
    ESP_LOGI(TAG, "Mock I2C transmit successful");
    return ESP_OK;
}

// Mock I2C master receive
esp_err_t mockI2cMasterReceive(i2c_master_dev_handle_t deviceHandle, uint8_t* readBuffer, size_t readSize, uint32_t timeoutMs) {
    ESP_LOGI(TAG, "Mock I2C receive called for %zu bytes", readSize);
    
    if (deviceHandle == NULL || readBuffer == NULL) {
        ESP_LOGE(TAG, "Mock I2C receive with NULL parameters");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Determine which sensor is being read based on device handle
    uint8_t deviceAddress = *(uint8_t*)deviceHandle;
    
    if (deviceAddress == MOCK_AHT21_ADDRESS && mockAht21Connected) {
        // Return AHT21 mock data
        size_t copySize = (readSize < sizeof(mockAht21Response)) ? readSize : sizeof(mockAht21Response);
        memcpy(readBuffer, mockAht21Response, copySize);
        ESP_LOGI(TAG, "Mock I2C receive: AHT21 data returned");
    } else if (deviceAddress == MOCK_ENS160_ADDRESS && mockEns160Connected) {
        // Return ENS160 mock data
        size_t copySize = (readSize < sizeof(mockEns160Data)) ? readSize : sizeof(mockEns160Data);
        memcpy(readBuffer, mockEns160Data, copySize);
        ESP_LOGI(TAG, "Mock I2C receive: ENS160 data returned");
    } else {
        // Simulate device not responding
        ESP_LOGW(TAG, "Mock I2C receive: Device not responding");
        return ESP_ERR_INVALID_STATE;
    }
    
    return ESP_OK;
}

// Mock I2C master transmit and receive
esp_err_t mockI2cMasterTransmitReceive(i2c_master_dev_handle_t deviceHandle, 
                                      const uint8_t* writeBuffer, size_t writeSize,
                                      uint8_t* readBuffer, size_t readSize, 
                                      uint32_t timeoutMs) {
    ESP_LOGI(TAG, "Mock I2C transmit-receive called");
    
    // First do the transmit
    esp_err_t transmitResult = mockI2cMasterTransmit(deviceHandle, writeBuffer, writeSize, timeoutMs);
    if (transmitResult != ESP_OK) {
        return transmitResult;
    }
    
    // Then do the receive
    return mockI2cMasterReceive(deviceHandle, readBuffer, readSize, timeoutMs);
}

// Mock I2C master multi-buffer transmit
esp_err_t mockI2cMasterMultiBufferTransmit(i2c_master_dev_handle_t deviceHandle, 
                                          const uint8_t* writeBuffer, size_t writeSize,
                                          uint32_t timeoutMs) {
    ESP_LOGI(TAG, "Mock I2C multi-buffer transmit called");
    return mockI2cMasterTransmit(deviceHandle, writeBuffer, writeSize, timeoutMs);
}

// Mock I2C master multi-buffer transmit and receive
esp_err_t mockI2cMasterMultiBufferTransmitReceive(i2c_master_dev_handle_t deviceHandle,
                                                 const uint8_t* writeBuffer, size_t writeSize,
                                                 uint8_t* readBuffer, size_t readSize,
                                                 uint32_t timeoutMs) {
    ESP_LOGI(TAG, "Mock I2C multi-buffer transmit-receive called");
    return mockI2cMasterTransmitReceive(deviceHandle, writeBuffer, writeSize, readBuffer, readSize, timeoutMs);
}

// Mock I2C bus scan
esp_err_t mockI2cMasterBusScan(i2c_master_bus_handle_t busHandle, uint8_t startAddress, uint8_t endAddress, uint8_t* deviceList, size_t deviceListSize) {
    ESP_LOGI(TAG, "Mock I2C bus scan called from 0x%02X to 0x%02X", startAddress, endAddress);
    
    if (busHandle == NULL || deviceList == NULL) {
        ESP_LOGE(TAG, "Mock I2C bus scan with NULL parameters");
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t deviceCount = 0;
    
    // Check for AHT21
    if (MOCK_AHT21_ADDRESS >= startAddress && MOCK_AHT21_ADDRESS <= endAddress && mockAht21Connected) {
        if (deviceCount < deviceListSize) {
            deviceList[deviceCount++] = MOCK_AHT21_ADDRESS;
            ESP_LOGI(TAG, "Mock I2C scan: Found AHT21 at 0x%02X", MOCK_AHT21_ADDRESS);
        }
    }
    
    // Check for ENS160
    if (MOCK_ENS160_ADDRESS >= startAddress && MOCK_ENS160_ADDRESS <= endAddress && mockEns160Connected) {
        if (deviceCount < deviceListSize) {
            deviceList[deviceCount++] = MOCK_ENS160_ADDRESS;
            ESP_LOGI(TAG, "Mock I2C scan: Found ENS160 at 0x%02X", MOCK_ENS160_ADDRESS);
        }
    }
    
    ESP_LOGI(TAG, "Mock I2C bus scan found %zu devices", deviceCount);
    return ESP_OK;
}

// Mock I2C device status check
esp_err_t mockI2cMasterDeviceProbe(i2c_master_dev_handle_t deviceHandle, uint32_t timeoutMs) {
    ESP_LOGI(TAG, "Mock I2C device probe called");
    
    if (deviceHandle == NULL) {
        ESP_LOGE(TAG, "Mock I2C device probe with NULL handle");
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t deviceAddress = *(uint8_t*)deviceHandle;
    
    if ((deviceAddress == MOCK_AHT21_ADDRESS && mockAht21Connected) ||
        (deviceAddress == MOCK_ENS160_ADDRESS && mockEns160Connected)) {
        ESP_LOGI(TAG, "Mock I2C device probe: Device at 0x%02X is responding", deviceAddress);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Mock I2C device probe: Device at 0x%02X not responding", deviceAddress);
        return ESP_ERR_INVALID_STATE;
    }
}

// Mock control functions for test setup
void mockI2cSetAht21Connected(bool connected) {
    mockAht21Connected = connected;
    ESP_LOGI(TAG, "Mock I2C: AHT21 connection status set to %s", connected ? "connected" : "disconnected");
}

void mockI2cSetEns160Connected(bool connected) {
    mockEns160Connected = connected;
    ESP_LOGI(TAG, "Mock I2C: ENS160 connection status set to %s", connected ? "connected" : "disconnected");
}

void mockI2cResetAllDevices(void) {
    mockAht21Connected = true;
    mockEns160Connected = true;
    ESP_LOGI(TAG, "Mock I2C: All devices reset to connected state");
} 