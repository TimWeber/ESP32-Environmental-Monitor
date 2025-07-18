#pragma once

#include <cstdint>
#include <stdexcept>

// Include the header that defines sensorDatabaseData_t
extern "C" {
    #include "network/http_client.h"
}

// Forward declarations for sensor data structures
struct AHT21Data;
struct ENS160Data;

// Function declarations
extern "C" {
    // Main application entry point 
    void app_main();
    
    // External C functions
    bool sendSensorDataToDatabase(const sensorDatabaseData_t* sensorData,
                                 uint32_t timeoutMs, 
                                 const char* serverUrl,
                                 uint8_t aqiMin, uint8_t aqiMax,
                                 uint16_t tvocMin, uint16_t tvocMax,
                                 uint16_t eco2Min, uint16_t eco2Max);
}


