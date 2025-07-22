#pragma once

#include <cstdint>
#include <atomic>
#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/**
 * @brief System statistics structure for health monitoring
 */
struct SystemStats {
    // Real-time counters (updated continuously)
    uint32_t sensor_readings_total{0};
    uint32_t sensor_readings_failed{0};
    uint32_t network_transmissions_total{0};
    uint32_t network_transmissions_failed{0};
    uint32_t watchdog_resets{0};
    
    // Current status (snapshot when requested)
    uint32_t uptime_seconds{0};
    uint32_t memory_free_bytes{0};
    int8_t wifi_signal_dbm{0};
    uint32_t last_reset_timestamp{0};
    
    // Sensor-specific health
    bool aht21_connected{false};
    bool ens160_connected{false};
    uint32_t aht21_success_rate{0};
    uint32_t ens160_success_rate{0};
    uint32_t zero_readings_count{0};
    
    // Sensor states
    std::string aht21_state{"unknown"};
    std::string ens160_state{"unknown"};
    
    // Network performance
    uint32_t http_success_rate{0};
    uint32_t average_response_time_ms{0};
    uint32_t retry_attempts_total{0};
    
    SystemStats() = default;
    
    // Copy constructor for atomic values
    SystemStats(const SystemStats& other) {
        sensor_readings_total = other.sensor_readings_total;
        sensor_readings_failed = other.sensor_readings_failed;
        network_transmissions_total = other.network_transmissions_total;
        network_transmissions_failed = other.network_transmissions_failed;
        watchdog_resets = other.watchdog_resets;
        uptime_seconds = other.uptime_seconds;
        memory_free_bytes = other.memory_free_bytes;
        wifi_signal_dbm = other.wifi_signal_dbm;
        last_reset_timestamp = other.last_reset_timestamp;
        aht21_connected = other.aht21_connected;
        ens160_connected = other.ens160_connected;
        aht21_success_rate = other.aht21_success_rate;
        ens160_success_rate = other.ens160_success_rate;
        zero_readings_count = other.zero_readings_count;
        aht21_state = other.aht21_state;
        ens160_state = other.ens160_state;
        http_success_rate = other.http_success_rate;
        average_response_time_ms = other.average_response_time_ms;
        retry_attempts_total = other.retry_attempts_total;
    }
    
    // Assignment operator
    SystemStats& operator=(const SystemStats& other) {
        if (this != &other) {
            sensor_readings_total = other.sensor_readings_total;
            sensor_readings_failed = other.sensor_readings_failed;
            network_transmissions_total = other.network_transmissions_total;
            network_transmissions_failed = other.network_transmissions_failed;
            watchdog_resets = other.watchdog_resets;
            uptime_seconds = other.uptime_seconds;
            memory_free_bytes = other.memory_free_bytes;
            wifi_signal_dbm = other.wifi_signal_dbm;
            last_reset_timestamp = other.last_reset_timestamp;
            aht21_connected = other.aht21_connected;
            ens160_connected = other.ens160_connected;
            aht21_success_rate = other.aht21_success_rate;
            ens160_success_rate = other.ens160_success_rate;
            zero_readings_count = other.zero_readings_count;
            aht21_state = other.aht21_state;
            ens160_state = other.ens160_state;
            http_success_rate = other.http_success_rate;
            average_response_time_ms = other.average_response_time_ms;
            retry_attempts_total = other.retry_attempts_total;
        }
        return *this;
    }
};

/**
 * @brief Health monitoring system for ESP32 Environmental Monitor
 * 
 * Collects system statistics in real-time and provides them on-demand
 * when a health check is requested via HTTP GET.
 */
class HealthMonitor {
private:
    static constexpr const char* TAG = "HealthMonitor";
    
    SystemStats stats_;
    std::string device_id_;
    TickType_t start_time_;
    SemaphoreHandle_t stats_mutex_;
    
    // Health check endpoint
    static constexpr const char* HEALTH_ENDPOINT = "/api/health";
    
public:
    /**
     * @brief Constructor
     * @param device_id Unique device identifier
     */
    explicit HealthMonitor(const std::string& device_id);
    
    /**
     * @brief Initialise the health monitoring system
     */
    void initialise();
    
    /**
     * @brief Increment sensor reading counter
     * @param success true if reading was successful, false otherwise
     */
    void recordSensorReading(bool success);
    
    /**
     * @brief Increment network transmission counter
     * @param success true if transmission was successful, false otherwise
     */
    void recordNetworkTransmission(bool success);
    
    /**
     * @brief Record a watchdog reset
     */
    void recordWatchdogReset();
    
    /**
     * @brief Update sensor connection status
     * @param aht21_connected AHT21 connection status
     * @param ens160_connected ENS160 connection status
     */
    void updateSensorStatus(bool aht21_connected, bool ens160_connected);
    
    /**
     * @brief Update sensor success rates
     * @param aht21_success_rate AHT21 success rate percentage
     * @param ens160_success_rate ENS160 success rate percentage
     */
    void updateSensorSuccessRates(uint32_t aht21_success_rate, uint32_t ens160_success_rate);
    
    /**
     * @brief Update zero readings count
     * @param count Number of zero readings (valid for clean air)
     */
    void updateZeroReadingsCount(uint32_t count);
    
    /**
     * @brief Update sensor states
     * @param aht21_state AHT21 sensor state (e.g., "ready", "error")
     * @param ens160_state ENS160 sensor state (e.g., "warm_up", "ready", "error")
     */
    void updateSensorStates(const std::string& aht21_state, const std::string& ens160_state);
    
    /**
     * @brief Update network performance metrics
     * @param http_success_rate HTTP success rate percentage
     * @param average_response_time_ms Average response time in milliseconds
     * @param retry_attempts_total Total retry attempts
     */
    void updateNetworkMetrics(uint32_t http_success_rate, uint32_t average_response_time_ms, uint32_t retry_attempts_total);
    
    /**
     * @brief Get current system statistics
     * @return SystemStats structure with current values
     */
    SystemStats getCurrentStats();
    
    /**
     * @brief Generate JSON health response
     * @return JSON string with system health data
     */
    std::string generateHealthResponse();
    
    /**
     * @brief Get health endpoint URL
     * @return Health endpoint string
     */
    static const char* getHealthEndpoint() { return HEALTH_ENDPOINT; }
    
    /**
     * @brief Handle health check request
     * @param request_data HTTP request data
     * @return JSON response string
     */
    std::string handleHealthCheck(const std::string& request_data);
}; 