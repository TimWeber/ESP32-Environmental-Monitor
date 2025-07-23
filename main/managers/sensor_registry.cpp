#include "sensor_registry.hpp"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include <sstream>
#include <iomanip>
#include <random>

const char* SensorRegistry::TAG = "SensorRegistry";
const char* SensorRegistry::NVS_NAMESPACE = "sensor_registry";
const char* SensorRegistry::SENSOR_ID_KEY = "sensor_id";
const char* SensorRegistry::REGISTRATION_TIMESTAMP_KEY = "reg_timestamp";
const char* SensorRegistry::SERVER_URL_KEY = "server_url";

SensorRegistry::SensorRegistry(std::shared_ptr<ConfigManager> configManager)
    : registration_timestamp_(0), is_registered_(false), config_manager_(configManager) {
}

SensorRegistry::~SensorRegistry() {
}

esp_err_t SensorRegistry::initialise() {
    ESP_LOGI(TAG, "Initialising sensor registry...");
    
    // Read existing data from NVS
    esp_err_t err = readFromNVS();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read from NVS, will create new sensor ID");
        err = generateNewSensorId();
        if (err == ESP_OK) {
            err = writeToNVS();
        }
    }
    
    if (err == ESP_OK) {
        // If we have a config manager and no server URL, load it from configuration
        if (config_manager_ && server_url_.empty()) {
            server_url_ = config_manager_->getServerUrl();
            if (!server_url_.empty()) {
                ESP_LOGI(TAG, "Loaded server URL from configuration: %s", server_url_.c_str());
                // Save the loaded URL to NVS for future use
                writeToNVS();
            } else {
                ESP_LOGW(TAG, "No server URL found in configuration");
            }
        } else if (!config_manager_) {
            ESP_LOGW(TAG, "No configuration manager provided - server URL must be set manually");
        }
        
        ESP_LOGI(TAG, "Sensor registry initialised - ID: %s, Registered: %s", 
                 sensor_id_.c_str(), is_registered_ ? "yes" : "no");
    } else {
        ESP_LOGE(TAG, "Failed to initialise sensor registry: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t SensorRegistry::readFromNVS() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read sensor ID
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, SENSOR_ID_KEY, nullptr, &required_size);
    if (err == ESP_OK && required_size > 0) {
        char* sensor_id_buffer = new char[required_size];
        err = nvs_get_str(nvs_handle, SENSOR_ID_KEY, sensor_id_buffer, &required_size);
        if (err == ESP_OK) {
            sensor_id_ = std::string(sensor_id_buffer);
            is_registered_ = true;
        }
        delete[] sensor_id_buffer;
    }
    
    // Read registration timestamp
    err = nvs_get_u64(nvs_handle, REGISTRATION_TIMESTAMP_KEY, &registration_timestamp_);
    if (err != ESP_OK) {
        registration_timestamp_ = 0;
    }
    
    // Read server URL
    required_size = 0;
    err = nvs_get_str(nvs_handle, SERVER_URL_KEY, nullptr, &required_size);
    if (err == ESP_OK && required_size > 0) {
        char* url_buffer = new char[required_size];
        err = nvs_get_str(nvs_handle, SERVER_URL_KEY, url_buffer, &required_size);
        if (err == ESP_OK) {
            server_url_ = std::string(url_buffer);
        }
        delete[] url_buffer;
    } else {
        // Server URL not found in NVS - will be set later from configuration
        ESP_LOGW(TAG, "Server URL not found in NVS - will be set from configuration");
    }
    
    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t SensorRegistry::writeToNVS() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write sensor ID
    err = nvs_set_str(nvs_handle, SENSOR_ID_KEY, sensor_id_.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write sensor ID to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Write registration timestamp
    err = nvs_set_u64(nvs_handle, REGISTRATION_TIMESTAMP_KEY, registration_timestamp_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write timestamp to NVS: %s", esp_err_to_name(err));
    }
    
    // Write server URL
    err = nvs_set_str(nvs_handle, SERVER_URL_KEY, server_url_.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write server URL to NVS: %s", esp_err_to_name(err));
    }
    
    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(err));
    }
    
    nvs_close(nvs_handle);
    return err;
}

esp_err_t SensorRegistry::generateNewSensorId() {
    // Generate a unique sensor ID based on MAC address + persistent timestamp
    uint8_t mac[6];
    esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get MAC address: %s", esp_err_to_name(err));
        return err;
    }
    
    // Get or create persistent timestamp (stored in NVS)
    uint64_t persistent_timestamp;
    nvs_handle_t nvs_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        // Try to read existing timestamp
        err = nvs_get_u64(nvs_handle, "persistent_ts", &persistent_timestamp);
        if (err != ESP_OK) {
            // No existing timestamp, create new one
            persistent_timestamp = esp_timer_get_time() / 1000000; // Convert to seconds
            nvs_set_u64(nvs_handle, "persistent_ts", persistent_timestamp);
            nvs_commit(nvs_handle);
            ESP_LOGI(TAG, "Created new persistent timestamp: %llu", persistent_timestamp);
        } else {
            ESP_LOGI(TAG, "Using existing persistent timestamp: %llu", persistent_timestamp);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGW(TAG, "Failed to open NVS for timestamp, using current time");
        persistent_timestamp = esp_timer_get_time() / 1000000;
    }
    
    // Create sensor ID: sensor_XXXX_YYYY (where XXXX is last 4 chars of MAC, YYYY is persistent timestamp)
    std::stringstream ss;
    ss << "sensor_";
    for (int i = 2; i < 6; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)mac[i];
    }
    ss << "_" << std::hex << std::setw(4) << std::setfill('0') << (persistent_timestamp & 0xFFFF); // Last 4 hex digits of persistent timestamp
    
    sensor_id_ = ss.str();
    registration_timestamp_ = persistent_timestamp;
    is_registered_ = false;
    
    ESP_LOGI(TAG, "Generated sensor ID: %s (MAC: %02x:%02x:%02x:%02x:%02x:%02x, persistent_timestamp: %llu)", 
             sensor_id_.c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], persistent_timestamp);
    return ESP_OK;
}

std::string SensorRegistry::getSensorId() {
    if (sensor_id_.empty()) {
        ESP_LOGW(TAG, "Sensor ID is empty, generating new one");
        generateNewSensorId();
        writeToNVS();
    }
    return sensor_id_;
}

esp_err_t SensorRegistry::resetSensorId() {
    ESP_LOGI(TAG, "Resetting sensor ID");
    return generateNewSensorId();
}

esp_err_t SensorRegistry::setServerUrl(const std::string& url) {
    server_url_ = url;
    return writeToNVS();
} 