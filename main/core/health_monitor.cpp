#include "health_monitor.hpp"
#include "cJSON.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include <sstream>
#include <iomanip>

HealthMonitor::HealthMonitor(const std::string& device_id)
    : device_id_(device_id), start_time_(xTaskGetTickCount()) {
    // Create mutex for thread safety
    stats_mutex_ = xSemaphoreCreateMutex();
    if (!stats_mutex_) {
        ESP_LOGE(TAG, "Failed to create stats mutex");
    }
    
    ESP_LOGI(TAG, "HealthMonitor created for device: %s", device_id_.c_str());
}

void HealthMonitor::initialise() {
    ESP_LOGI(TAG, "Initialising health monitoring system");
    start_time_ = xTaskGetTickCount();
    
    // Initialise statistics with mutex protection
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(1000)) == pdTRUE) {
        stats_.sensor_readings_total = 0;
        stats_.sensor_readings_failed = 0;
        stats_.network_transmissions_total = 0;
        stats_.network_transmissions_failed = 0;
        stats_.watchdog_resets = 0;
        xSemaphoreGive(stats_mutex_);
    }
    
    ESP_LOGI(TAG, "Health monitoring system initialised");
}

void HealthMonitor::recordSensorReading(bool success) {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (success) {
            stats_.sensor_readings_total++;
        } else {
            stats_.sensor_readings_failed++;
        }
        xSemaphoreGive(stats_mutex_);
    }
}

void HealthMonitor::recordNetworkTransmission(bool success) {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (success) {
            stats_.network_transmissions_total++;
        } else {
            stats_.network_transmissions_failed++;
        }
        xSemaphoreGive(stats_mutex_);
    }
}

void HealthMonitor::recordWatchdogReset() {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        stats_.watchdog_resets++;
        xSemaphoreGive(stats_mutex_);
        ESP_LOGW(TAG, "Watchdog reset recorded - total resets: %lu", stats_.watchdog_resets);
    }
}

void HealthMonitor::updateSensorStatus(bool aht21_connected, bool ens160_connected) {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        stats_.aht21_connected = aht21_connected;
        stats_.ens160_connected = ens160_connected;
        xSemaphoreGive(stats_mutex_);
    }
    
    ESP_LOGD(TAG, "Sensor status updated - AHT21: %s, ENS160: %s",
              aht21_connected ? "connected" : "disconnected",
              ens160_connected ? "connected" : "disconnected");
}

void HealthMonitor::updateSensorSuccessRates(uint32_t aht21_success_rate, uint32_t ens160_success_rate) {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        stats_.aht21_success_rate = aht21_success_rate;
        stats_.ens160_success_rate = ens160_success_rate;
        xSemaphoreGive(stats_mutex_);
    }
}

void HealthMonitor::updateZeroReadingsCount(uint32_t count) {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        stats_.zero_readings_count = count;
        xSemaphoreGive(stats_mutex_);
    }
}

void HealthMonitor::updateSensorStates(const std::string& aht21_state, const std::string& ens160_state) {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        stats_.aht21_state = aht21_state;
        stats_.ens160_state = ens160_state;
        xSemaphoreGive(stats_mutex_);
    }
    
    ESP_LOGD(TAG, "Sensor states updated - AHT21: %s, ENS160: %s",
              aht21_state.c_str(), ens160_state.c_str());
}

void HealthMonitor::updateNetworkMetrics(uint32_t http_success_rate, uint32_t average_response_time_ms, uint32_t retry_attempts_total) {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        stats_.http_success_rate = http_success_rate;
        stats_.average_response_time_ms = average_response_time_ms;
        stats_.retry_attempts_total = retry_attempts_total;
        xSemaphoreGive(stats_mutex_);
    }
}

SystemStats HealthMonitor::getCurrentStats() {
    SystemStats current_stats;
    
    // Get a snapshot of stats with mutex protection
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(1000)) == pdTRUE) {
        current_stats = stats_;
        xSemaphoreGive(stats_mutex_);
    }
    
    // Calculate uptime
    TickType_t current_time = xTaskGetTickCount();
    current_stats.uptime_seconds = pdTICKS_TO_MS(current_time - start_time_) / 1000;
    
    // Get memory information
    current_stats.memory_free_bytes = esp_get_free_heap_size();
    
    // Get WiFi signal strength
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        current_stats.wifi_signal_dbm = ap_info.rssi;
    } else {
        current_stats.wifi_signal_dbm = -100; // Indicates no connection
    }
    
    // Get last reset timestamp
    current_stats.last_reset_timestamp = esp_timer_get_time() / 1000000; // Convert to seconds
    
    return current_stats;
}

std::string HealthMonitor::generateHealthResponse() {
    SystemStats stats = getCurrentStats();
    
    cJSON* root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON root object");
        return "{}";
    }
    
    // Device information
    cJSON_AddStringToObject(root, "device", device_id_.c_str());
    
    // Timestamp using ESP-IDF time functions
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    cJSON_AddStringToObject(root, "timestamp", timestamp);
    
    // Uptime
    cJSON_AddNumberToObject(root, "uptime_seconds", stats.uptime_seconds);
    
    // System health
    cJSON* system_health = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "system_health", system_health);
    cJSON_AddNumberToObject(system_health, "sensor_readings_total", stats.sensor_readings_total);
    cJSON_AddNumberToObject(system_health, "sensor_readings_failed", stats.sensor_readings_failed);
    cJSON_AddNumberToObject(system_health, "network_transmissions_total", stats.network_transmissions_total);
    cJSON_AddNumberToObject(system_health, "network_transmissions_failed", stats.network_transmissions_failed);
    cJSON_AddNumberToObject(system_health, "memory_free_bytes", stats.memory_free_bytes);
    cJSON_AddNumberToObject(system_health, "wifi_signal_dbm", stats.wifi_signal_dbm);
    cJSON_AddNumberToObject(system_health, "watchdog_resets", stats.watchdog_resets);
    
    // Sensor health
    cJSON* sensor_health = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "sensor_health", sensor_health);
    cJSON_AddBoolToObject(sensor_health, "aht21_connected", stats.aht21_connected);
    cJSON_AddBoolToObject(sensor_health, "ens160_connected", stats.ens160_connected);
    cJSON_AddNumberToObject(sensor_health, "aht21_success_rate", stats.aht21_success_rate);
    cJSON_AddNumberToObject(sensor_health, "ens160_success_rate", stats.ens160_success_rate);
    cJSON_AddNumberToObject(sensor_health, "zero_readings_count", stats.zero_readings_count);
    cJSON_AddStringToObject(sensor_health, "aht21_state", stats.aht21_state.c_str());
    cJSON_AddStringToObject(sensor_health, "ens160_state", stats.ens160_state.c_str());
    
    // Network health
    cJSON* network_health = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "network_health", network_health);
    cJSON_AddNumberToObject(network_health, "http_success_rate", stats.http_success_rate);
    cJSON_AddNumberToObject(network_health, "average_response_time_ms", stats.average_response_time_ms);
    cJSON_AddNumberToObject(network_health, "retry_attempts_total", stats.retry_attempts_total);
    
    // Generate JSON string
    char* json_string = cJSON_Print(root);
    std::string result(json_string ? json_string : "{}");
    
    // Cleanup
    if (json_string) {
        free(json_string);
    }
    cJSON_Delete(root);
    
    ESP_LOGD(TAG, "Generated health response: %s", result.c_str());
    return result;
}

std::string HealthMonitor::handleHealthCheck(const std::string& request_data) {
    ESP_LOGI(TAG, "Health check requested");
    
    // Log the request for debugging
    ESP_LOGD(TAG, "Health check request data: %s", request_data.c_str());
    
    // Generate and return health response
    std::string response = generateHealthResponse();
    
    ESP_LOGI(TAG, "Health check response generated successfully");
    return response;
} 