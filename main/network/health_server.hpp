#pragma once

#include "core/health_monitor.hpp"
#include <memory>
#include <string>

/**
 * @brief HTTP server handler for health monitoring endpoints
 * 
 * Handles GET requests to /api/health and responds with system statistics
 */
class HealthServer {
private:
    static constexpr const char* TAG = "HealthServer";
    std::shared_ptr<HealthMonitor> health_monitor_;
    
public:
    /**
     * @brief Constructor
     * @param health_monitor Shared pointer to health monitor instance
     */
    explicit HealthServer(std::shared_ptr<HealthMonitor> health_monitor);
    
    /**
     * @brief Handle HTTP GET request for health check
     * @param request_data Raw HTTP request data
     * @return HTTP response string with JSON health data
     */
    std::string handleHealthRequest(const std::string& request_data);
    
    /**
     * @brief Check if request is for health endpoint
     * @param request_data Raw HTTP request data
     * @return true if request is for health endpoint, false otherwise
     */
    bool isHealthRequest(const std::string& request_data);
    
    /**
     * @brief Get health endpoint URL
     * @return Health endpoint string
     */
    static const char* getHealthEndpoint() { return HealthMonitor::getHealthEndpoint(); }
}; 