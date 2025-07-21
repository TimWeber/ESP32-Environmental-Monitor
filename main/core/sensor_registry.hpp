#pragma once

#include <string>
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

class SensorRegistry {
private:
    static const char* TAG;
    static const char* NVS_NAMESPACE;
    static const char* SENSOR_ID_KEY;
    static const char* REGISTRATION_TIMESTAMP_KEY;
    static const char* SERVER_URL_KEY;
    
    std::string sensor_id_;
    std::string server_url_;
    uint64_t registration_timestamp_;
    bool is_registered_;
    
    esp_err_t readFromNVS();
    esp_err_t writeToNVS();
    esp_err_t generateNewSensorId();
    
public:
    SensorRegistry();
    ~SensorRegistry();
    
    // Initialise the registry
    esp_err_t initialise();
    
    // Get or create sensor ID
    std::string getSensorId();
    
    // Check if sensor is registered
    bool isRegistered() const { return is_registered_; }
    
    // Get server URL
    std::string getServerUrl() const { return server_url_; }
    
    // Get registration timestamp
    uint64_t getRegistrationTimestamp() const { return registration_timestamp_; }
    
    // Force new sensor ID (for testing)
    esp_err_t resetSensorId();
    
    // Update server URL
    esp_err_t setServerUrl(const std::string& url);
}; 