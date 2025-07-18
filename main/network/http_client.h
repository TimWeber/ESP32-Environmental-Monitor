#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temperature;
    float humidity;
    bool aht21Valid;
    uint8_t aqi;
    uint16_t tvoc;
    uint16_t eco2;
    bool ens160Valid;
} sensorDatabaseData_t;

// Consolidated function that handles all sensor data transmission scenarios
// Parameters:
// - sensorData: pointer to sensor data structure (can be NULL for basic temp/humidity)
// - timeoutMs: HTTP timeout in milliseconds (0 = use default 15000ms)
// - serverUrl: server URL (NULL = use default from config)
// - aqiMin/Max: AQI validation thresholds (0 = use defaults)
// - tvocMin/Max: TVOC validation thresholds (0 = use defaults)  
// - eco2Min/Max: eCO2 validation thresholds (0 = use defaults)
bool sendSensorDataToDatabase(const sensorDatabaseData_t *sensorData,
                             uint32_t timeoutMs, 
                             const char* serverUrl,
                             uint8_t aqiMin, uint8_t aqiMax,
                             uint16_t tvocMin, uint16_t tvocMax,
                             uint16_t eco2Min, uint16_t eco2Max);



#ifdef __cplusplus
}
#endif
