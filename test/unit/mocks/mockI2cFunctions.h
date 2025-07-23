/*
 * Mock I2C Functions for ESP32 Environmental Monitor Tests
 * 
 * This header defines mock functions for I2C operations used by sensors.
 * These mocks allow testing sensor functionality without real hardware.
 */

#ifndef MOCK_I2C_FUNCTIONS_H
#define MOCK_I2C_FUNCTIONS_H

#include "esp_err.h"
#include "driver/i2c.h"
#include <stdint.h>
#include <stddef.h>

// Mock initialization and cleanup
void mockI2cFunctionsInit(void);
void mockI2cFunctionsCleanup(void);

// Mock I2C master initialisation
esp_err_t mockI2cMasterInit(const i2c_config_t* config);

// Mock I2C master device creation
i2c_master_bus_handle_t mockI2cMasterBusAddDevice(i2c_master_bus_handle_t busHandle, uint8_t deviceAddress);

// Mock I2C master device removal
esp_err_t mockI2cMasterBusRemoveDevice(i2c_master_bus_handle_t busHandle, i2c_master_dev_handle_t deviceHandle);

// Mock I2C master transmit
esp_err_t mockI2cMasterTransmit(i2c_master_dev_handle_t deviceHandle, const uint8_t* writeBuffer, size_t writeSize, uint32_t timeoutMs);

// Mock I2C master receive
esp_err_t mockI2cMasterReceive(i2c_master_dev_handle_t deviceHandle, uint8_t* readBuffer, size_t readSize, uint32_t timeoutMs);

// Mock I2C master transmit and receive
esp_err_t mockI2cMasterTransmitReceive(i2c_master_dev_handle_t deviceHandle, 
                                      const uint8_t* writeBuffer, size_t writeSize,
                                      uint8_t* readBuffer, size_t readSize, 
                                      uint32_t timeoutMs);

// Mock I2C master multi-buffer transmit
esp_err_t mockI2cMasterMultiBufferTransmit(i2c_master_dev_handle_t deviceHandle, 
                                          const uint8_t* writeBuffer, size_t writeSize,
                                          uint32_t timeoutMs);

// Mock I2C master multi-buffer transmit and receive
esp_err_t mockI2cMasterMultiBufferTransmitReceive(i2c_master_dev_handle_t deviceHandle,
                                                 const uint8_t* writeBuffer, size_t writeSize,
                                                 uint8_t* readBuffer, size_t readSize,
                                                 uint32_t timeoutMs);

// Mock I2C bus scan
esp_err_t mockI2cMasterBusScan(i2c_master_bus_handle_t busHandle, uint8_t startAddress, uint8_t endAddress, uint8_t* deviceList, size_t deviceListSize);

// Mock I2C device status check
esp_err_t mockI2cMasterDeviceProbe(i2c_master_dev_handle_t deviceHandle, uint32_t timeoutMs);

#endif // MOCK_I2C_FUNCTIONS_H 