#include "health_server.hpp"
#include "esp_log.h"
#include <algorithm>

HealthServer::HealthServer(std::shared_ptr<HealthMonitor> health_monitor)
    : health_monitor_(health_monitor) {
    ESP_LOGI(TAG, "HealthServer created");
}

std::string HealthServer::handleHealthRequest(const std::string& request_data) {
    ESP_LOGI(TAG, "Handling health request");
    
    if (!health_monitor_) {
        ESP_LOGE(TAG, "Health monitor not available");
        return "{\"error\": \"Health monitor not available\"}";
    }
    
    // Generate health response
    std::string json_response = health_monitor_->handleHealthCheck(request_data);
    
    ESP_LOGI(TAG, "Health response generated successfully");
    return json_response;
}

bool HealthServer::isHealthRequest(const std::string& request_data) {
    // Check if request starts with GET and contains health endpoint
    std::string upper_request = request_data;
    std::transform(upper_request.begin(), upper_request.end(), upper_request.begin(), ::toupper);
    
    const std::string health_endpoint = HealthMonitor::getHealthEndpoint();
    
    // Look for GET request to health endpoint
    if (upper_request.find("GET " + std::string(health_endpoint)) != std::string::npos) {
        ESP_LOGD(TAG, "Health request detected");
        return true;
    }
    
    return false;
} 