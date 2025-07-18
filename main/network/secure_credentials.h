#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Secure credential management for WiFi and other sensitive data
 * 
 * This module provides secure storage and retrieval of credentials using NVS.
 * Credentials are stored encrypted in non-volatile storage and can be set
 * programmatically without hardcoding them in source code.
 */

// WiFi credential structure
typedef struct {
    char ssid[33];        // WiFi SSID (max 32 chars + null terminator)
    char password[65];    // WiFi password (max 64 chars + null terminator)
    bool is_set;          // Whether credentials have been set
} wifi_credentials_t;

// Server credential structure
typedef struct {
    char server_url[256]; // Server URL
    char auth_token[128]; // Authentication token (if needed)
    bool is_set;          // Whether credentials have been set
} server_credentials_t;

/**
 * @brief Initialize the secure credential system
 * @return ESP_OK on success, error code on failure
 */
esp_err_t secure_credentials_init(void);

/**
 * @brief Set WiFi credentials securely
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return ESP_OK on success, error code on failure
 */
esp_err_t secure_credentials_set_wifi(const char* ssid, const char* password);

/**
 * @brief Get WiFi credentials
 * @param credentials Pointer to store the credentials
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not set
 */
esp_err_t secure_credentials_get_wifi(wifi_credentials_t* credentials);

/**
 * @brief Set server credentials securely
 * @param server_url Server URL
 * @param auth_token Authentication token (optional, can be NULL)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t secure_credentials_set_server(const char* server_url, const char* auth_token);

/**
 * @brief Get server credentials
 * @param credentials Pointer to store the credentials
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not set
 */
esp_err_t secure_credentials_get_server(server_credentials_t* credentials);

/**
 * @brief Check if WiFi credentials are set
 * @return true if credentials are available, false otherwise
 */
bool secure_credentials_has_wifi(void);

/**
 * @brief Check if server credentials are set
 * @return true if credentials are available, false otherwise
 */
bool secure_credentials_has_server(void);

/**
 * @brief Clear all stored credentials (for factory reset)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t secure_credentials_clear_all(void);

/**
 * @brief Load credentials from JSON config file (for development/testing)
 * @param config_path Path to JSON configuration file
 * @return ESP_OK on success, error code on failure
 */
esp_err_t secure_credentials_load_from_config(const char* config_path);

#ifdef __cplusplus
}
#endif 