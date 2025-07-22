#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// HTTP server configuration
#define HTTP_SERVER_PORT 80
#define HTTP_SERVER_MAX_CONNECTIONS 4

/**
 * @brief Initialise HTTP server for health monitoring
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_server_init(void);

/**
 * @brief Deinitialise HTTP server
 */
void http_server_deinit(void);

/**
 * @brief Start HTTP server
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_server_start(void);

/**
 * @brief Stop HTTP server
 */
void http_server_stop(void);

/**
 * @brief Register health monitoring callback
 * 
 * @param callback Function to call when health request is received
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t http_server_register_health_callback(const char* (*callback)(const char* request_data));

#ifdef __cplusplus
}
#endif 