#include "wifi_manager.h"
#include "secure_credentials.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include <string.h>

static const char* TAG = "WiFi";

// Lightweight WiFi manager optimised for deep sleep applications
static EventGroupHandle_t wifi_event_group = NULL;
static char current_ip[16] = "0.0.0.0";
static bool wifi_initialised = false;

#define WIFI_MAXIMUM_RETRY          3
#define WIFI_CONNECTION_TIMEOUT_MS  15000

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    static int retry_count = 0;
    
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WiFi station connected to AP");
                retry_count = 0; // Reset retry counter on successful connection
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected from AP");
                
                if (retry_count < WIFI_MAXIMUM_RETRY) {
                    retry_count++;
                    ESP_LOGI(TAG, "Retrying... (%d/%d)", retry_count, WIFI_MAXIMUM_RETRY);
                    esp_wifi_connect();
                } else {
                    ESP_LOGE(TAG, "Max retries reached");
                    if (wifi_event_group) {
                        xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
                    }
                }
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        snprintf(current_ip, sizeof(current_ip), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", current_ip);
        
        if (wifi_event_group) {
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

// Initialise WiFi - lightweight implementation for deep sleep compatibility
esp_err_t wifi_manager_init(void)
{
    if (wifi_initialised) {
        ESP_LOGW(TAG, "WiFi manager already initialised");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Init WiFi manager");
    
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

    // Get WiFi credentials from secure storage
    wifi_credentials_t credentials;
    esp_err_t cred_err = secure_credentials_get_wifi(&credentials);
    if (cred_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi credentials: %s", esp_err_to_name(cred_err));
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!credentials.is_set) {
        ESP_LOGE(TAG, "WiFi credentials not set");
        return ESP_ERR_INVALID_STATE;
    }
    
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    // Copy SSID and password from secure credentials
    strncpy((char*)wifi_config.sta.ssid, credentials.ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    strncpy((char*)wifi_config.sta.password, credentials.password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_initialised = true;
    ESP_LOGI(TAG, "WiFi init done");
    return ESP_OK;
}

// Connect to WiFi and await connection with timeout
bool wifi_manager_connect_and_wait(uint32_t timeout_ms)
{
    if (!wifi_initialised) {
        ESP_LOGE(TAG, "WiFi manager not initialised");
        return false;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi with %lu ms timeout", timeout_ms);
    
    // Clear previous connection state
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    strcpy(current_ip, "0.0.0.0");
    
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE, pdFALSE,
                                          pdMS_TO_TICKS(timeout_ms));
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully. IP: %s", current_ip);
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Connection failed");
        return false;
    } else {
        ESP_LOGE(TAG, "WiFi connection timeout after %lu ms", timeout_ms);
        return false;
    }
}

// Check if WiFi is connected
bool wifi_manager_is_connected(void)
{
    if (!wifi_initialised || !wifi_event_group) {
        return false;
    }
    
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

// Get current IP address
const char* wifi_manager_get_ip(void)
{
    return current_ip;
}

// Disconnect from WiFi in preparation for sleep
void wifi_manager_disconnect(void)
{
    if (wifi_initialised) {
        ESP_LOGI(TAG, "Disconnecting WiFi for deep sleep");
        esp_wifi_disconnect();
        strcpy(current_ip, "0.0.0.0");
    }
}

// Clean up WiFi manager resources
void wifi_manager_deinit(void)
{
    if (!wifi_initialised) {
        return;
    }
    
    ESP_LOGI(TAG, "Deinitialising WiFi manager");
    
    // Disconnect if connected
    wifi_manager_disconnect();
    
    // Clean up event group
    if (wifi_event_group) {
        vEventGroupDelete(wifi_event_group);
        wifi_event_group = NULL;
    }
    
    wifi_initialised = false;
    ESP_LOGI(TAG, "WiFi manager deinitialised");
}

// Config-based initialization functions
esp_err_t wifi_manager_init_from_config(const char* config_path)
{
    ESP_LOGI(TAG, "Initializing WiFi from config: %s", config_path);
    
    // Load credentials from config file into secure storage
    if (config_path) {
        esp_err_t config_result = secure_credentials_load_from_config(config_path);
        if (config_result != ESP_OK) {
            ESP_LOGW(TAG, "Failed to load credentials from config: %s", esp_err_to_name(config_result));
        }
    }
    
    // Get WiFi credentials from secure storage
    wifi_credentials_t credentials;
    esp_err_t cred_err = secure_credentials_get_wifi(&credentials);
    if (cred_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi credentials: %s", esp_err_to_name(cred_err));
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!credentials.is_set) {
        ESP_LOGE(TAG, "WiFi credentials not set");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Using WiFi SSID: %s", credentials.ssid);
    
    if (wifi_initialised) {
        ESP_LOGW(TAG, "WiFi manager already initialised");
        return ESP_OK;
    }
    
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    // Copy SSID and password from secure credentials
    strncpy((char*)wifi_config.sta.ssid, credentials.ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    strncpy((char*)wifi_config.sta.password, credentials.password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_initialised = true;
    ESP_LOGI(TAG, "WiFi init done");
    return ESP_OK;
}

bool wifi_manager_connect_and_wait_from_config(const char* config_path)
{
    ESP_LOGI(TAG, "Connecting WiFi from config: %s", config_path);
    

    uint32_t timeout_ms = WIFI_CONNECTION_TIMEOUT_MS;
    
    ESP_LOGI(TAG, "Using WiFi timeout: %lu ms", timeout_ms);
    return wifi_manager_connect_and_wait(timeout_ms);
}

// Add this function to get ESP32 IP address
const char* wifi_get_ip_string(void) {
    static char ip_str[16];
    esp_netif_ip_info_t ip_info;
    
    if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK) {
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
        return ip_str;
    }
    
    return "0.0.0.0";
}

// Add this to your WiFi connection success callback
void wifi_connection_success_callback(void) {
    ESP_LOGI("WIFI", "Connected to WiFi");
    ESP_LOGI("WIFI", "ESP32 IP Address: %s", wifi_get_ip_string());
    ESP_LOGI("WIFI", "Health endpoint available at: http://%s/api/health", wifi_get_ip_string());
}

