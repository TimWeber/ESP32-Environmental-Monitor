#include "SensorManager.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>



SensorManager::SensorManager(std::shared_ptr<I2CManager> i2cManager)
    : i2cManager_(i2cManager), configPath_(nullptr), initialised_(false), 
      totalReadings_(0), failedReadings_(0), lastHealthCheck_(0) {
    ESP_LOGI(TAG, "SensorManager created");
}

bool SensorManager::initialise() {
    ESP_LOGI(TAG, "Initialising SensorManager");
    
    if (!i2cManager_) {
        ESP_LOGE(TAG, "I2C manager not provided");
        return false;
    }
    
    if (!i2cManager_->isInitialised()) {
        ESP_LOGE(TAG, "I2C manager not initialised");
        return false;
    }
    
    initialised_ = true;
    ESP_LOGI(TAG, "SensorManager initialised successfully");
    return true;
}

bool SensorManager::initialiseFromConfig(const char* configPath) {
    ESP_LOGI(TAG, "Initialising SensorManager from config: %s", configPath);
    
    if (!initialise()) {
        ESP_LOGE(TAG, "Failed to initialise SensorManager");
        return false;
    }
    
    if (!loadSensorConfiguration()) {
        ESP_LOGW(TAG, "Failed to load sensor configuration");
        // Don't fail completely, continue with default settings
    }
    
    return true;
}

bool SensorManager::addSensor(const std::string& name, std::shared_ptr<ISensor> sensor) {
    if (!initialised_) {
        ESP_LOGE(TAG, "SensorManager not initialised");
        return false;
    }
    
    if (!sensor) {
        ESP_LOGE(TAG, "Invalid sensor pointer for sensor: %s", name.c_str());
        return false;
    }
    
    if (sensors_.find(name) != sensors_.end()) {
        ESP_LOGW(TAG, "Sensor already exists: %s", name.c_str());
        return false;
    }
    
    // Initialize the sensor
    if (!sensor->initialise()) {
        ESP_LOGW(TAG, "Failed to initialise sensor: %s", name.c_str());
        return false;
    }
    
    sensors_[name] = sensor;
    sensorHealth_[name] = SensorHealthStatus();
    sensorHealth_[name].sensorName = name;
    
    ESP_LOGI(TAG, "Sensor added successfully: %s", name.c_str());
    return true;
}

bool SensorManager::removeSensor(const std::string& name) {
    auto it = sensors_.find(name);
    if (it == sensors_.end()) {
        ESP_LOGW(TAG, "Sensor not found: %s", name.c_str());
        return false;
    }
    
    sensors_.erase(it);
    sensorHealth_.erase(name);
    sensorConfigs_.erase(name);
    
    ESP_LOGI(TAG, "Sensor removed: %s", name.c_str());
    return true;
}

bool SensorManager::enableSensor(const std::string& name) {
    auto it = sensorConfigs_.find(name);
    if (it == sensorConfigs_.end()) {
        ESP_LOGW(TAG, "Sensor config not found: %s", name.c_str());
        return false;
    }
    
    it->second.enabled = true;
    ESP_LOGI(TAG, "Sensor enabled: %s", name.c_str());
    return true;
}

bool SensorManager::disableSensor(const std::string& name) {
    auto it = sensorConfigs_.find(name);
    if (it == sensorConfigs_.end()) {
        ESP_LOGW(TAG, "Sensor config not found: %s", name.c_str());
        return false;
    }
    
    it->second.enabled = false;
    ESP_LOGI(TAG, "Sensor disabled: %s", name.c_str());
    return true;
}

UnifiedSensorData SensorManager::readAllSensors() {
    UnifiedSensorData data;
    data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    data.isNewData = false;
    
    if (!initialised_) {
        ESP_LOGW(TAG, "SensorManager not initialised");
        return data;
    }
    
    for (const auto& [name, sensor] : sensors_) {
        if (!isSensorActive(name)) {
            continue;
        }
        
        try {
            SensorReading reading = sensor->readData();
            data.valid[name] = reading.valid;
            
            if (reading.valid) {
                data.isNewData = true;
                totalReadings_++;
                
                // Add all values from the reading to the unified data
                for (const auto& [field, value] : reading.values) {
                    data.values[field] = value;
                }
                
                ESP_LOGD(TAG, "Sensor %s read successfully", name.c_str());
                updateSensorHealth(name, true);
            } else {
                failedReadings_++;
                ESP_LOGW(TAG, "Sensor %s returned invalid data", name.c_str());
                updateSensorHealth(name, false);
            }
        } catch (const std::exception& e) {
            failedReadings_++;
            ESP_LOGE(TAG, "Error reading sensor %s: %s", name.c_str(), e.what());
            data.valid[name] = false;
            updateSensorHealth(name, false);
        }
    }
    
    return data;
}

SensorReading SensorManager::readSensor(const std::string& name) {
    SensorReading reading;
    reading.valid = false;
    
    if (!initialised_) {
        ESP_LOGW(TAG, "SensorManager not initialised");
        return reading;
    }
    
    auto it = sensors_.find(name);
    if (it == sensors_.end()) {
        ESP_LOGW(TAG, "Sensor not found: %s", name.c_str());
        return reading;
    }
    
    if (!isSensorActive(name)) {
        ESP_LOGW(TAG, "Sensor not active: %s", name.c_str());
        return reading;
    }
    
    try {
        reading = it->second->readData();
        if (reading.valid) {
            totalReadings_++;
            updateSensorHealth(name, true);
        } else {
            failedReadings_++;
            updateSensorHealth(name, false);
        }
    } catch (const std::exception& e) {
        failedReadings_++;
        ESP_LOGE(TAG, "Error reading sensor %s: %s", name.c_str(), e.what());
        updateSensorHealth(name, false);
    }
    
    return reading;
}

void SensorManager::checkAllSensorHealth() {
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    lastHealthCheck_ = currentTime;
    
    for (auto& [name, health] : sensorHealth_) {
        auto it = sensors_.find(name);
        if (it == sensors_.end()) {
            health.isHealthy = false;
            health.status = "sensor_not_found";
            continue;
        }
        
        try {
            bool isReady = it->second->isReady();
            if (isReady) {
                health.isHealthy = true;
                health.consecutiveFailures = 0;
                health.status = "healthy";
            } else {
                health.isHealthy = false;
                health.consecutiveFailures++;
                health.status = "not_ready";
            }
        } catch (const std::exception& e) {
            health.isHealthy = false;
            health.consecutiveFailures++;
            health.status = "error: " + std::string(e.what());
        }
        
        health.lastCheckTime = currentTime;
    }
}

SensorHealthStatus SensorManager::getSensorHealth(const std::string& name) const {
    auto it = sensorHealth_.find(name);
    if (it == sensorHealth_.end()) {
        return SensorHealthStatus();
    }
    return it->second;
}

std::map<std::string, SensorHealthStatus> SensorManager::getAllSensorHealth() const {
    return sensorHealth_;
}

std::vector<std::string> SensorManager::getAllSensors() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : sensors_) {
        names.push_back(name);
    }
    return names;
}

std::vector<std::string> SensorManager::getActiveSensors() const {
    std::vector<std::string> activeSensors;
    for (const auto& [name, _] : sensors_) {
        if (isSensorActive(name)) {
            activeSensors.push_back(name);
        }
    }
    return activeSensors;
}

size_t SensorManager::getActiveSensorCount() const {
    size_t count = 0;
    for (const auto& [name, _] : sensors_) {
        if (isSensorActive(name)) {
            count++;
        }
    }
    return count;
}

bool SensorManager::hasSensor(const std::string& name) const {
    return sensors_.find(name) != sensors_.end();
}

bool SensorManager::isSensorActive(const std::string& name) const {
    auto it = sensorConfigs_.find(name);
    if (it == sensorConfigs_.end()) {
        // If no config exists, assume sensor is active
        return true;
    }
    return it->second.enabled;
}

SensorConfig SensorManager::getSensorConfig(const std::string& name) const {
    auto it = sensorConfigs_.find(name);
    if (it == sensorConfigs_.end()) {
        return SensorConfig();
    }
    return it->second;
}



bool SensorManager::updateSensorConfig(const std::string& name, const SensorConfig& config) {
    if (!validateSensorConfiguration(config)) {
        ESP_LOGE(TAG, "Invalid sensor configuration for: %s", name.c_str());
        return false;
    }
    
    sensorConfigs_[name] = config;
    ESP_LOGI(TAG, "Sensor configuration updated: %s", name.c_str());
    return true;
}

void SensorManager::getStatistics(uint32_t& totalReadings, uint32_t& failedReadings) const {
    totalReadings = totalReadings_;
    failedReadings = failedReadings_;
}

void SensorManager::resetStatistics() {
    totalReadings_ = 0;
    failedReadings_ = 0;
    ESP_LOGI(TAG, "SensorManager statistics reset");
}

// Private methods
bool SensorManager::loadSensorConfiguration() {
    // For now, return true as we don't have a config file system implemented
    // This can be extended later to load from JSON or other config files
    ESP_LOGI(TAG, "Loading sensor configuration (default implementation)");
    return true;
}

bool SensorManager::validateSensorConfiguration(const SensorConfig& config) {
    if (config.name.empty()) {
        ESP_LOGE(TAG, "Sensor name cannot be empty");
        return false;
    }
    
    if (config.intervalMs < 100) {
        ESP_LOGE(TAG, "Sensor interval too short: %lu ms", config.intervalMs);
        return false;
    }
    
    return true;
}

void SensorManager::updateSensorHealth(const std::string& sensorName, bool success) {
    auto it = sensorHealth_.find(sensorName);
    if (it == sensorHealth_.end()) {
        return;
    }
    
    if (success) {
        it->second.consecutiveFailures = 0;
        it->second.isHealthy = true;
    } else {
        it->second.consecutiveFailures++;
        if (it->second.consecutiveFailures > 5) {
            it->second.isHealthy = false;
        }
    }
}

std::vector<std::string> SensorManager::getEnabledSensors() const {
    std::vector<std::string> enabledSensors;
    for (const auto& [name, config] : sensorConfigs_) {
        if (config.enabled) {
            enabledSensors.push_back(name);
        }
    }
    return enabledSensors;
} 