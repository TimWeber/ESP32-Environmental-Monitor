#pragma once

#include <string>
#include <memory>
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "config_manager.hpp"

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
    std::shared_ptr<ConfigManager> config_manager_;
    
    esp_err_t readFromNVS();
    esp_err_t writeToNVS();
    esp_err_t generateNewSensorId();
    
public:
    // Constructor with configuration dependency
    explicit SensorRegistry(std::shared_ptr<ConfigManager> configManager);
    
    ~SensorRegistry();
    
    // Initialise the registry
    esp_err_t initialise();
    
    // Get or create sensor ID
    std::string getSensorId();
    
    // Check if sensor is registered
    bool isRegistered() const { return is_registered_; }
    
    // Get server URL
    std::string getServerUrl() const { return server_url_; }
    
    // Check if server URL is configured
    bool isServerUrlConfigured() const { return !server_url_.empty(); }
    
    // Check if registry is properly configured
    bool isConfigured() const { return !sensor_id_.empty() && !server_url_.empty(); }
    
    // Get registration timestamp
    uint64_t getRegistrationTimestamp() const { return registration_timestamp_; }
    
    // Force new sensor ID (for testing)
    esp_err_t resetSensorId();
    
    // Update server URL
    esp_err_t setServerUrl(const std::string& url);
}; 