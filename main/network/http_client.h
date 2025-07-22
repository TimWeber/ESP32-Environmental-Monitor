#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device name for HTTP requests
extern const char* deviceName;

// Server URL for data transmission
extern const char* serverUrl;

// Sensor data structure for database transmission
typedef struct {
    float temperature;
    float humidity;
    uint8_t aqi;
    uint16_t tvoc;
    uint16_t eco2;
    bool aht21Valid;
    bool ens160Valid;
    bool isNewData;
} sensorDatabaseData_t;

/**
 * @brief Send sensor data to database with configurable parameters
 * 
 * @param sensorData Pointer to sensor data structure
 * @param timeoutMs HTTP timeout in milliseconds (0 for default)
 * @param serverUrlParam Custom server URL (NULL for default)
 * @param aqiMin Minimum AQI value (0 for no minimum)
 * @param aqiMax Maximum AQI value (0 for no maximum)
 * @param tvocMin Minimum TVOC value (0 for no minimum)
 * @param tvocMax Maximum TVOC value (0 for no maximum)
 * @param eco2Min Minimum eCO2 value (0 for no minimum)
 * @param eco2Max Maximum eCO2 value (0 for no maximum)
 * @return true if transmission successful, false otherwise
 */
bool sendSensorDataToDatabase(const sensorDatabaseData_t *sensorData,
                             uint32_t timeoutMs, 
                             const char* serverUrlParam,
                             uint8_t aqiMin, uint8_t aqiMax,
                             uint16_t tvocMin, uint16_t tvocMax,
                             uint16_t eco2Min, uint16_t eco2Max);

/**
 * @brief Send sensor data to database with default parameters
 * 
 * @param sensorData Pointer to sensor data structure
 * @return true if transmission successful, false otherwise
 */
bool sendSensorDataToDatabaseDefault(const sensorDatabaseData_t *sensorData);

/**
 * @brief Send handshake to server
 * 
 * @param sensorId Sensor ID
 * @param esp32Ip ESP32 IP address
 * @param serverUrl Server URL for handshake
 * @param timeoutMs HTTP timeout in milliseconds (0 for default)
 * @return true if handshake successful, false otherwise
 */
bool sendHandshakeToServer(const char* sensorId, const char* esp32Ip, const char* serverUrl, uint32_t timeoutMs);

/**
 * @brief Initialise HTTP client configuration
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_client_init(void);

/**
 * @brief Deinitialise HTTP client
 */
void http_client_deinit(void);

#ifdef __cplusplus
}
#endif
