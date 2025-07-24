#include "config_manager.hpp"
#include "core/calibration.hpp"
#include "core/json_config_provider.hpp"
#include "esp_log.h"
#include <cstring>

static const char* TAG = "ConfigManager";

ConfigManager::ConfigManager(std::unique_ptr<IConfigProvider> provider)
    : configProvider_(std::move(provider)),
      sensorReadingIntervalMs_(DEFAULT_SENSOR_INTERVAL_MS),
      networkTransmissionIntervalMs_(DEFAULT_NETWORK_INTERVAL_MS),
      monitorReportIntervalMs_(DEFAULT_MONITOR_INTERVAL_MS),
      heartbeatIntervalSeconds_(DEFAULT_HEARTBEAT_SECONDS),
      sensorQueueSize_(DEFAULT_SENSOR_QUEUE_SIZE),
      detailedLoggingEnabled_(false),
      sleepModeEnabled_(false),
      networkMaxRetries_(DEFAULT_NETWORK_MAX_RETRIES),
      networkRetryDelayMs_(DEFAULT_NETWORK_RETRY_DELAY_MS),
      networkTimeoutMs_(DEFAULT_NETWORK_TIMEOUT_MS),
      i2cSdaPin_(DEFAULT_I2C_SDA_PIN),
      i2cSclPin_(DEFAULT_I2C_SCL_PIN),
      aqiMin_(DEFAULT_AQI_MIN),
      aqiMax_(DEFAULT_AQI_MAX),
      tvocMin_(DEFAULT_TVOC_MIN),
      tvocMax_(DEFAULT_TVOC_MAX),
      eco2Min_(DEFAULT_ECO2_MIN),
      eco2Max_(DEFAULT_ECO2_MAX),
      ens160WarmupTimeoutMs_(DEFAULT_ENS160_WARMUP_TIMEOUT_MS),
      calibrationManager_(std::make_unique<CalibrationManager>()) {
    
    ESP_LOGI(TAG, "ConfigManager created");
}

bool ConfigManager::loadDataCollectionConfig() {
    if (!configProvider_) {
        ESP_LOGW(TAG, "No config provider available");
        return false;
    }
    
    auto dataCollectionSection = configProvider_->getSection("data_collection");
    if (!dataCollectionSection) {
        ESP_LOGW(TAG, "No data_collection section found in config");
        return false;
    }
    
    // Load sensor reading interval
    int readingIntervalMinutes;
    if (dataCollectionSection->getInt("sensor_reading_interval_minutes", readingIntervalMinutes)) {
        sensorReadingIntervalMs_ = readingIntervalMinutes * 60 * 1000;
        ESP_LOGI(TAG, "Loaded sensor reading interval: %d minutes (%lu ms)", 
                 readingIntervalMinutes, sensorReadingIntervalMs_);
    }
    
    // Load hardware configuration
    auto hardwareSection = dataCollectionSection->getSection("hardware");
    if (hardwareSection) {
        auto i2cSection = hardwareSection->getSection("i2c");
        if (i2cSection) {
            int sdaPin, sclPin;
            if (i2cSection->getInt("sda_pin", sdaPin)) {
                i2cSdaPin_ = sdaPin;
                ESP_LOGI(TAG, "Loaded I2C SDA pin: %d", i2cSdaPin_);
            }
            if (i2cSection->getInt("scl_pin", sclPin)) {
                i2cSclPin_ = sclPin;
                ESP_LOGI(TAG, "Loaded I2C SCL pin: %d", i2cSclPin_);
            }
        }
    }
    
    // Load calibration equations
    auto calibrationSection = dataCollectionSection->getSection("calibration");
    if (calibrationSection) {
        // Load temperature calibration
        auto tempSection = calibrationSection->getSection("temperature");
        if (tempSection) {
            std::string type, unit;
            if (tempSection->getString("type", type) && tempSection->getString("unit", unit)) {
                if (type == "offset") {
                    float value;
                    if (tempSection->getFloat("value", value)) {
                        calibrationManager_->setTemperatureCalibration(
                            std::make_unique<OffsetCalibration>(value, unit));
                        ESP_LOGI(TAG, "Loaded temperature offset calibration: %.2f %s", value, unit.c_str());
                    }
                } else if (type == "linear") {
                    float slope, intercept;
                    if (tempSection->getFloat("slope", slope) && tempSection->getFloat("intercept", intercept)) {
                        calibrationManager_->setTemperatureCalibration(
                            std::make_unique<LinearCalibration>(slope, intercept, unit));
                        ESP_LOGI(TAG, "Loaded temperature linear calibration: y = %.2fx + %.2f %s", 
                                 slope, intercept, unit.c_str());
                    }
                } else if (type == "polynomial") {
                    // For polynomial, we'd need to load coefficients array
                    // This is a simplified version - in practice you'd parse the coefficients
                    ESP_LOGI(TAG, "Polynomial calibration not yet implemented for temperature");
                }
            }
        }
        
        // Load humidity calibration
        auto humiditySection = calibrationSection->getSection("humidity");
        if (humiditySection) {
            std::string type, unit;
            if (humiditySection->getString("type", type) && humiditySection->getString("unit", unit)) {
                if (type == "offset") {
                    float value;
                    if (humiditySection->getFloat("value", value)) {
                        calibrationManager_->setHumidityCalibration(
                            std::make_unique<OffsetCalibration>(value, unit));
                        ESP_LOGI(TAG, "Loaded humidity offset calibration: %.2f %s", value, unit.c_str());
                    }
                } else if (type == "linear") {
                    float slope, intercept;
                    if (humiditySection->getFloat("slope", slope) && humiditySection->getFloat("intercept", intercept)) {
                        calibrationManager_->setHumidityCalibration(
                            std::make_unique<LinearCalibration>(slope, intercept, unit));
                        ESP_LOGI(TAG, "Loaded humidity linear calibration: y = %.2fx + %.2f %s", 
                                 slope, intercept, unit.c_str());
                    }
                }
            }
        }
        
        // Load eCO2 calibration
        auto eco2Section = calibrationSection->getSection("eco2");
        if (eco2Section) {
            std::string type, unit;
            if (eco2Section->getString("type", type) && eco2Section->getString("unit", unit)) {
                if (type == "offset") {
                    int value;
                    if (eco2Section->getInt("value", value)) {
                        calibrationManager_->setEco2Calibration(
                            std::make_unique<OffsetCalibration>(static_cast<float>(value), unit));
                        ESP_LOGI(TAG, "Loaded eCO2 offset calibration: %d %s", value, unit.c_str());
                    }
                } else if (type == "linear") {
                    float slope, intercept;
                    if (eco2Section->getFloat("slope", slope) && eco2Section->getFloat("intercept", intercept)) {
                        calibrationManager_->setEco2Calibration(
                            std::make_unique<LinearCalibration>(slope, intercept, unit));
                        ESP_LOGI(TAG, "Loaded eCO2 linear calibration: y = %.2fx + %.2f %s", 
                                 slope, intercept, unit.c_str());
                    }
                }
            }
        }
        
        // Load TVOC calibration
        auto tvocSection = calibrationSection->getSection("tvoc");
        if (tvocSection) {
            std::string type, unit;
            if (tvocSection->getString("type", type) && tvocSection->getString("unit", unit)) {
                if (type == "offset") {
                    int value;
                    if (tvocSection->getInt("value", value)) {
                        calibrationManager_->setTvocCalibration(
                            std::make_unique<OffsetCalibration>(static_cast<float>(value), unit));
                        ESP_LOGI(TAG, "Loaded TVOC offset calibration: %d %s", value, unit.c_str());
                    }
                } else if (type == "linear") {
                    float slope, intercept;
                    if (tvocSection->getFloat("slope", slope) && tvocSection->getFloat("intercept", intercept)) {
                        calibrationManager_->setTvocCalibration(
                            std::make_unique<LinearCalibration>(slope, intercept, unit));
                        ESP_LOGI(TAG, "Loaded TVOC linear calibration: y = %.2fx + %.2f %s", 
                                 slope, intercept, unit.c_str());
                    }
                }
            }
        }
    }
    
    // Load data validation thresholds
    auto dataValidationSection = dataCollectionSection->getSection("data_validation");
    if (dataValidationSection) {
        // Air quality thresholds
        auto airQualitySection = dataValidationSection->getSection("air_quality");
        if (airQualitySection) {
            int aqiMin, aqiMax;
            ESP_LOGI(TAG, "Attempting to read aqi_min from config...");
            if (airQualitySection->getInt("aqi_min", aqiMin)) {
                aqiMin_ = static_cast<uint8_t>(aqiMin);
                ESP_LOGI(TAG, "Loaded AQI minimum: %d", aqiMin_);
            } else {
                ESP_LOGW(TAG, "Failed to read aqi_min from config, using default: %d", aqiMin_);
            }
            ESP_LOGI(TAG, "Attempting to read aqi_max from config...");
            if (airQualitySection->getInt("aqi_max", aqiMax)) {
                aqiMax_ = static_cast<uint8_t>(aqiMax);
                ESP_LOGI(TAG, "Loaded AQI maximum: %d", aqiMax_);
            } else {
                ESP_LOGW(TAG, "Failed to read aqi_max from config, using default: %d", aqiMax_);
            }
        }
        
        // Volatile compounds thresholds
        auto volatileCompoundsSection = dataValidationSection->getSection("volatile_compounds");
        if (volatileCompoundsSection) {
            int tvocMin, tvocMax;
            if (volatileCompoundsSection->getInt("tvoc_min_ppb", tvocMin)) {
                tvocMin_ = static_cast<uint16_t>(tvocMin);
                ESP_LOGI(TAG, "Loaded TVOC minimum: %d ppb", tvocMin_);
            }
            if (volatileCompoundsSection->getInt("tvoc_max_ppb", tvocMax)) {
                tvocMax_ = static_cast<uint16_t>(tvocMax);
                ESP_LOGI(TAG, "Loaded TVOC maximum: %d ppb", tvocMax_);
            }
        }
        
        // Carbon dioxide thresholds
        auto carbonDioxideSection = dataValidationSection->getSection("carbon_dioxide");
        if (carbonDioxideSection) {
            int eco2Min, eco2Max;
            if (carbonDioxideSection->getInt("eco2_min_ppm", eco2Min)) {
                eco2Min_ = static_cast<uint16_t>(eco2Min);
                ESP_LOGI(TAG, "Loaded eCO2 minimum: %d ppm", eco2Min_);
            }
            if (carbonDioxideSection->getInt("eco2_max_ppm", eco2Max)) {
                eco2Max_ = static_cast<uint16_t>(eco2Max);
                ESP_LOGI(TAG, "Loaded eCO2 maximum: %d ppm", eco2Max_);
            }
        }
    }
    
    return true;
}

bool ConfigManager::loadDataTransmissionConfig() {
    if (!configProvider_) {
        ESP_LOGW(TAG, "No config provider available");
        return false;
    }
    
    auto dataTransmissionSection = configProvider_->getSection("data_transmission");
    if (!dataTransmissionSection) {
        ESP_LOGW(TAG, "No data_transmission section found in config");
        return false;
    }
    
    // Load posting interval
    int postingIntervalMinutes;
    if (dataTransmissionSection->getInt("posting_interval_minutes", postingIntervalMinutes)) {
        networkTransmissionIntervalMs_ = postingIntervalMinutes * 60 * 1000;
        ESP_LOGI(TAG, "Loaded posting interval: %d minutes (%lu ms)", 
                 postingIntervalMinutes, networkTransmissionIntervalMs_);
    }
    
    // Load server configuration
    auto serverSection = dataTransmissionSection->getSection("server");
    if (serverSection) {
        std::string url;
        if (serverSection->getString("url", url)) {
            serverUrl_ = url;
            ESP_LOGI(TAG, "Loaded server URL: %s", serverUrl_.c_str());
        }
        
        int timeoutMs;
        if (serverSection->getInt("request_timeout_ms", timeoutMs)) {
            networkTimeoutMs_ = timeoutMs;
            ESP_LOGI(TAG, "Loaded request timeout: %lu ms", (unsigned long)networkTimeoutMs_);
        }
    }
    
    // Load retry policy
    auto retryPolicySection = dataTransmissionSection->getSection("retry_policy");
    if (retryPolicySection) {
        int maxAttempts, delayMs;
        if (retryPolicySection->getInt("max_attempts", maxAttempts)) {
            networkMaxRetries_ = maxAttempts;
            ESP_LOGI(TAG, "Loaded max retry attempts: %lu", (unsigned long)networkMaxRetries_);
        }
        if (retryPolicySection->getInt("delay_between_attempts_ms", delayMs)) {
            networkRetryDelayMs_ = delayMs;
            ESP_LOGI(TAG, "Loaded retry delay: %lu ms", (unsigned long)networkRetryDelayMs_);
        }
    }
    
    return true;
}

bool ConfigManager::loadConnectivityConfig() {
    if (!configProvider_) {
        ESP_LOGW(TAG, "No config provider available");
        return false;
    }
    
    auto connectivitySection = configProvider_->getSection("connectivity");
    if (!connectivitySection) {
        ESP_LOGW(TAG, "No connectivity section found in config");
        return false;
    }
    
    // Load WiFi configuration
    auto wifiSection = connectivitySection->getSection("wifi");
    if (wifiSection) {
        std::string ssid, password;
        if (wifiSection->getString("ssid", ssid)) {
            ESP_LOGI(TAG, "Loaded WiFi SSID: %s", ssid.c_str());
        }
        if (wifiSection->getString("password", password)) {
            ESP_LOGI(TAG, "Loaded WiFi password: [HIDDEN]");
        }
        
        int timeoutMs;
        if (wifiSection->getInt("connection_timeout_ms", timeoutMs)) {
            ESP_LOGI(TAG, "Loaded WiFi connection timeout: %d ms", timeoutMs);
        }
    }
    
    return true;
}

bool ConfigManager::loadSystemMonitoringConfig() {
    if (!configProvider_) {
        ESP_LOGW(TAG, "No config provider available");
        return false;
    }
    
    auto systemMonitoringSection = configProvider_->getSection("system_monitoring");
    if (!systemMonitoringSection) {
        ESP_LOGW(TAG, "No system_monitoring section found in config");
        return false;
    }
    
    // Load heartbeat interval
    int heartbeatSeconds;
    if (systemMonitoringSection->getInt("heartbeat_interval_seconds", heartbeatSeconds)) {
        heartbeatIntervalSeconds_ = heartbeatSeconds;
        ESP_LOGI(TAG, "Loaded heartbeat interval: %lu seconds", (unsigned long)heartbeatIntervalSeconds_);
    }
    
    // Load status report interval
    int statusMinutes;
    if (systemMonitoringSection->getInt("status_report_interval_minutes", statusMinutes)) {
        monitorReportIntervalMs_ = statusMinutes * 60 * 1000;
        ESP_LOGI(TAG, "Loaded status report interval: %d minutes (%lu ms)", 
                 statusMinutes, monitorReportIntervalMs_);
    }
    
    // Load performance settings
    auto performanceSection = systemMonitoringSection->getSection("performance");
    if (performanceSection) {
        int queueSize;
        if (performanceSection->getInt("sensor_queue_size", queueSize)) {
            sensorQueueSize_ = queueSize;
            ESP_LOGI(TAG, "Loaded sensor queue size: %lu", (unsigned long)sensorQueueSize_);
        }
        
        if (performanceSection->hasKey("detailed_logging_enabled")) {
            performanceSection->getBool("detailed_logging_enabled", detailedLoggingEnabled_);
            ESP_LOGI(TAG, "Loaded detailed logging setting: %s", 
                     detailedLoggingEnabled_ ? "enabled" : "disabled");
        }
        
        if (performanceSection->hasKey("sleep_mode_enabled")) {
            performanceSection->getBool("sleep_mode_enabled", sleepModeEnabled_);
            ESP_LOGI(TAG, "Loaded sleep mode setting: %s", 
                     sleepModeEnabled_ ? "enabled" : "disabled");
        }
    }
    
    return true;
}

bool ConfigManager::loadSensorConfig() {
    if (!configProvider_) {
        ESP_LOGW(TAG, "No config provider available");
        return false;
    }
    
    auto dataCollectionSection = configProvider_->getSection("data_collection");
    if (!dataCollectionSection) {
        ESP_LOGW(TAG, "No data_collection section found in config");
        return false;
    }
    
    // Load sensor-specific configuration
    auto sensorsSection = dataCollectionSection->getSection("sensor_parameters");
    if (sensorsSection) {
        auto ens160Section = sensorsSection->getSection("ens160");
        if (ens160Section) {
            int warmupTimeoutMinutes;
            if (ens160Section->getInt("warmup_timeout_minutes", warmupTimeoutMinutes)) {
                ens160WarmupTimeoutMs_ = warmupTimeoutMinutes * 60 * 1000; // Convert minutes to milliseconds
                ESP_LOGI(TAG, "Loaded ENS160 warmup timeout: %d minutes (%lu ms)", 
                         warmupTimeoutMinutes, ens160WarmupTimeoutMs_);
            }
        }
    }
    
    return true;
}

bool ConfigManager::validateConfiguration() const {
    ESP_LOGI(TAG, "Validating configuration...");
    
    bool isValid = true;
    std::string errorMsg;
    
    // Check if config provider exists
    if (!configProvider_) {
        ESP_LOGE(TAG, "Configuration validation failed: No config provider");
        return false;
    }
    
    // Check for template/default values that indicate configuration wasn't properly set up
    auto connectivitySection = configProvider_->getSection("connectivity");
    if (connectivitySection) {
        auto wifiSection = connectivitySection->getSection("wifi");
        if (wifiSection) {
            std::string ssid, password;
            if (wifiSection->getString("ssid", ssid)) {
                // Check for template values
                if (ssid == "YOUR_WIFI_SSID" || ssid.empty()) {
                    ESP_LOGE(TAG, "Configuration validation failed: WiFi SSID is template value or empty");
                    isValid = false;
                    errorMsg += "WiFi SSID not configured; ";
                }
            } else {
                ESP_LOGE(TAG, "Configuration validation failed: WiFi SSID not found");
                isValid = false;
                errorMsg += "WiFi SSID missing; ";
            }
            
            if (wifiSection->getString("password", password)) {
                // Check for template values
                if (password == "YOUR_WIFI_PASSWORD" || password.empty()) {
                    ESP_LOGE(TAG, "Configuration validation failed: WiFi password is template value or empty");
                    isValid = false;
                    errorMsg += "WiFi password not configured; ";
                }
            } else {
                ESP_LOGE(TAG, "Configuration validation failed: WiFi password not found");
                isValid = false;
                errorMsg += "WiFi password missing; ";
            }
        } else {
            ESP_LOGE(TAG, "Configuration validation failed: WiFi section not found");
            isValid = false;
            errorMsg += "WiFi section missing; ";
        }
    } else {
        ESP_LOGE(TAG, "Configuration validation failed: Connectivity section not found");
        isValid = false;
        errorMsg += "Connectivity section missing; ";
    }
    
    // Check server URL
    auto dataTransmissionSection = configProvider_->getSection("data_transmission");
    if (dataTransmissionSection) {
        auto serverSection = dataTransmissionSection->getSection("server");
        if (serverSection) {
            std::string url;
            if (serverSection->getString("url", url)) {
                // Check for template values
                if (url.find("your-server.local") != std::string::npos || 
                    url.find("YOUR_SERVER") != std::string::npos ||
                    url.empty()) {
                    ESP_LOGE(TAG, "Configuration validation failed: Server URL is template value or empty");
                    isValid = false;
                    errorMsg += "Server URL not configured; ";
                }
            } else {
                ESP_LOGE(TAG, "Configuration validation failed: Server URL not found");
                isValid = false;
                errorMsg += "Server URL missing; ";
            }
        } else {
            ESP_LOGE(TAG, "Configuration validation failed: Server section not found");
            isValid = false;
            errorMsg += "Server section missing; ";
        }
    } else {
        ESP_LOGE(TAG, "Configuration validation failed: Data transmission section not found");
        isValid = false;
        errorMsg += "Data transmission section missing; ";
    }
    
    // Check for reasonable sensor intervals
    auto dataCollectionSection = configProvider_->getSection("data_collection");
    if (dataCollectionSection) {
        int readingInterval;
        if (dataCollectionSection->getInt("sensor_reading_interval_minutes", readingInterval)) {
            if (readingInterval < 1 || readingInterval > 1440) { // 1 minute to 24 hours
                ESP_LOGE(TAG, "Configuration validation failed: Sensor reading interval out of range: %d minutes", readingInterval);
                isValid = false;
                errorMsg += "Invalid sensor reading interval; ";
            }
        } else {
            ESP_LOGE(TAG, "Configuration validation failed: Sensor reading interval not found");
            isValid = false;
            errorMsg += "Sensor reading interval missing; ";
        }
    } else {
        ESP_LOGE(TAG, "Configuration validation failed: Data collection section not found");
        isValid = false;
        errorMsg += "Data collection section missing; ";
    }
    
    if (isValid) {
        ESP_LOGI(TAG, "Configuration validation passed");
    } else {
        ESP_LOGE(TAG, "Configuration validation failed: %s", errorMsg.c_str());
        ESP_LOGE(TAG, "Please copy config/settings.template.jsonc to data/settings.json and configure with your actual values");
    }
    
    return isValid;
}

std::unique_ptr<IConfigProvider> createConfigProvider(const std::string& configPath) {
    if (configPath.empty()) {
        ESP_LOGW(TAG, "No config path provided");
        return nullptr;
    }
    
    // Check if it's a JSON file (simple extension check)
    if (configPath.find(".json") != std::string::npos) {
        ESP_LOGI(TAG, "Creating JSON config provider for: %s", configPath.c_str());
        return std::make_unique<JsonConfigProvider>(configPath);
    }
    
    ESP_LOGW(TAG, "Unsupported config file format: %s", configPath.c_str());
    return nullptr;
} 