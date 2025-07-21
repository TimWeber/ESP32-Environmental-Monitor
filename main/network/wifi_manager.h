#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// WiFi event bits for event groups
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT          BIT1

// WiFi connection status
typedef enum {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED
} wifi_status_t;

// Function declarations for deep sleep optimised WiFi manager
esp_err_t wifi_manager_init(void);
bool wifi_manager_connect_and_wait(uint32_t timeout_ms);
bool wifi_manager_is_connected(void);
const char* wifi_manager_get_ip(void);
void wifi_manager_disconnect(void);
void wifi_manager_deinit(void);

// Config-based initialization functions
esp_err_t wifi_manager_init_from_config(const char* config_path);
bool wifi_manager_connect_and_wait_from_config(const char* config_path);

/**
 * @brief Get ESP32 IP address as string
 * @return IP address string (e.g., "192.168.1.100")
 */
const char* wifi_get_ip_string(void);

/**
 * @brief WiFi connection success callback
 */
void wifi_connection_success_callback(void);
