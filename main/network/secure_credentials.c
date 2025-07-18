#include "secure_credentials.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char* TAG = "SecureCreds";
static const char* NVS_NAMESPACE = "credentials";
static const char* NVS_WIFI_SSID_KEY = "wifi_ssid";
static const char* NVS_WIFI_PASS_KEY = "wifi_pass";
static const char* NVS_SERVER_URL_KEY = "server_url";
static const char* NVS_SERVER_TOKEN_KEY = "server_token";

// NVS handle for credential storage
static nvs_handle_t nvs_cred_handle = 0;
static bool initialized = false;

esp_err_t secure_credentials_init(void) {
    if (initialized) {
        ESP_LOGW(TAG, "Secure credentials already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing secure credential storage");
    
    // Open NVS namespace for credentials
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_cred_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Secure credential storage initialized successfully");
    return ESP_OK;
}

esp_err_t secure_credentials_set_wifi(const char* ssid, const char* password) {
    if (!initialized) {
        ESP_LOGE(TAG, "Secure credentials not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!ssid || !password) {
        ESP_LOGE(TAG, "Invalid WiFi credentials (NULL pointer)");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(ssid) == 0 || strlen(ssid) > 32) {
        ESP_LOGE(TAG, "Invalid SSID length: %zu", strlen(ssid));
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(password) == 0 || strlen(password) > 64) {
        ESP_LOGE(TAG, "Invalid password length: %zu", strlen(password));
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Setting WiFi credentials for SSID: %s", ssid);
    
    // Store SSID
    esp_err_t err = nvs_set_str(nvs_cred_handle, NVS_WIFI_SSID_KEY, ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store SSID: %s", esp_err_to_name(err));
        return err;
    }
    
    // Store password
    err = nvs_set_str(nvs_cred_handle, NVS_WIFI_PASS_KEY, password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store password: %s", esp_err_to_name(err));
        return err;
    }
    
    // Commit changes
    err = nvs_commit(nvs_cred_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit WiFi credentials: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "WiFi credentials stored successfully");
    return ESP_OK;
}

esp_err_t secure_credentials_get_wifi(wifi_credentials_t* credentials) {
    if (!initialized) {
        ESP_LOGE(TAG, "Secure credentials not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!credentials) {
        ESP_LOGE(TAG, "Invalid credentials pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize credentials structure
    memset(credentials, 0, sizeof(wifi_credentials_t));
    
    // Get required size for SSID
    size_t ssid_size = 0;
    esp_err_t err = nvs_get_str(nvs_cred_handle, NVS_WIFI_SSID_KEY, NULL, &ssid_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SSID not found in NVS: %s", esp_err_to_name(err));
        return ESP_ERR_NOT_FOUND;
    }
    
    // Get required size for password
    size_t pass_size = 0;
    err = nvs_get_str(nvs_cred_handle, NVS_WIFI_PASS_KEY, NULL, &pass_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Password not found in NVS: %s", esp_err_to_name(err));
        return ESP_ERR_NOT_FOUND;
    }
    
    // Check if sizes fit in our structure
    if (ssid_size > sizeof(credentials->ssid) || pass_size > sizeof(credentials->password)) {
        ESP_LOGE(TAG, "Credential data too large for structure");
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Read SSID
    err = nvs_get_str(nvs_cred_handle, NVS_WIFI_SSID_KEY, credentials->ssid, &ssid_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read SSID: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read password
    err = nvs_get_str(nvs_cred_handle, NVS_WIFI_PASS_KEY, credentials->password, &pass_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read password: %s", esp_err_to_name(err));
        return err;
    }
    
    credentials->is_set = true;
    ESP_LOGI(TAG, "WiFi credentials retrieved successfully for SSID: %s", credentials->ssid);
    return ESP_OK;
}

esp_err_t secure_credentials_set_server(const char* server_url, const char* auth_token) {
    if (!initialized) {
        ESP_LOGE(TAG, "Secure credentials not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!server_url) {
        ESP_LOGE(TAG, "Invalid server URL (NULL pointer)");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(server_url) == 0 || strlen(server_url) > 255) {
        ESP_LOGE(TAG, "Invalid server URL length: %zu", strlen(server_url));
        return ESP_ERR_INVALID_ARG;
    }
    
    if (auth_token && (strlen(auth_token) == 0 || strlen(auth_token) > 127)) {
        ESP_LOGE(TAG, "Invalid auth token length: %zu", strlen(auth_token));
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Setting server credentials for URL: %s", server_url);
    
    // Store server URL
    esp_err_t err = nvs_set_str(nvs_cred_handle, NVS_SERVER_URL_KEY, server_url);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store server URL: %s", esp_err_to_name(err));
        return err;
    }
    
    // Store auth token if provided
    if (auth_token) {
        err = nvs_set_str(nvs_cred_handle, NVS_SERVER_TOKEN_KEY, auth_token);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to store auth token: %s", esp_err_to_name(err));
            return err;
        }
    } else {
        // Clear auth token if not provided
        err = nvs_erase_key(nvs_cred_handle, NVS_SERVER_TOKEN_KEY);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "Failed to clear auth token: %s", esp_err_to_name(err));
        }
    }
    
    // Commit changes
    err = nvs_commit(nvs_cred_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit server credentials: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Server credentials stored successfully");
    return ESP_OK;
}

esp_err_t secure_credentials_get_server(server_credentials_t* credentials) {
    if (!initialized) {
        ESP_LOGE(TAG, "Secure credentials not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!credentials) {
        ESP_LOGE(TAG, "Invalid credentials pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize credentials structure
    memset(credentials, 0, sizeof(server_credentials_t));
    
    // Get required size for server URL
    size_t url_size = 0;
    esp_err_t err = nvs_get_str(nvs_cred_handle, NVS_SERVER_URL_KEY, NULL, &url_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Server URL not found in NVS: %s", esp_err_to_name(err));
        return ESP_ERR_NOT_FOUND;
    }
    
    // Check if URL size fits in our structure
    if (url_size > sizeof(credentials->server_url)) {
        ESP_LOGE(TAG, "Server URL too large for structure");
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Read server URL
    err = nvs_get_str(nvs_cred_handle, NVS_SERVER_URL_KEY, credentials->server_url, &url_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read server URL: %s", esp_err_to_name(err));
        return err;
    }
    
    // Try to read auth token (optional)
    size_t token_size = 0;
    err = nvs_get_str(nvs_cred_handle, NVS_SERVER_TOKEN_KEY, NULL, &token_size);
    if (err == ESP_OK && token_size > 0 && token_size <= sizeof(credentials->auth_token)) {
        err = nvs_get_str(nvs_cred_handle, NVS_SERVER_TOKEN_KEY, credentials->auth_token, &token_size);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to read auth token: %s", esp_err_to_name(err));
            // Don't fail the whole operation for auth token
        }
    }
    
    credentials->is_set = true;
    ESP_LOGI(TAG, "Server credentials retrieved successfully for URL: %s", credentials->server_url);
    return ESP_OK;
}

bool secure_credentials_has_wifi(void) {
    if (!initialized) {
        return false;
    }
    
    size_t ssid_size = 0;
    esp_err_t err = nvs_get_str(nvs_cred_handle, NVS_WIFI_SSID_KEY, NULL, &ssid_size);
    if (err != ESP_OK) {
        return false;
    }
    
    size_t pass_size = 0;
    err = nvs_get_str(nvs_cred_handle, NVS_WIFI_PASS_KEY, NULL, &pass_size);
    return (err == ESP_OK);
}

bool secure_credentials_has_server(void) {
    if (!initialized) {
        return false;
    }
    
    size_t url_size = 0;
    esp_err_t err = nvs_get_str(nvs_cred_handle, NVS_SERVER_URL_KEY, NULL, &url_size);
    return (err == ESP_OK);
}

esp_err_t secure_credentials_clear_all(void) {
    if (!initialized) {
        ESP_LOGE(TAG, "Secure credentials not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Clearing all stored credentials");
    
    // Erase all credential keys
    nvs_erase_key(nvs_cred_handle, NVS_WIFI_SSID_KEY);
    nvs_erase_key(nvs_cred_handle, NVS_WIFI_PASS_KEY);
    nvs_erase_key(nvs_cred_handle, NVS_SERVER_URL_KEY);
    nvs_erase_key(nvs_cred_handle, NVS_SERVER_TOKEN_KEY);
    
    // Commit changes
    esp_err_t err = nvs_commit(nvs_cred_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit credential clearing: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "All credentials cleared successfully");
    return ESP_OK;
}

esp_err_t secure_credentials_load_from_config(const char* config_path) {
    if (!initialized) {
        ESP_LOGE(TAG, "Secure credentials not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!config_path) {
        ESP_LOGE(TAG, "Invalid config path");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Loading credentials from config file: %s", config_path);
    
    FILE* file = fopen(config_path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open config file: %s", config_path);
        return ESP_ERR_NOT_FOUND;
    }
    
    // Read file content
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        ESP_LOGE(TAG, "Failed to allocate memory for config file");
        return ESP_ERR_NO_MEM;
    }
    
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    
    // Parse JSON
    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON config file");
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t result = ESP_OK;
    
    // Load WiFi credentials from connectivity.wifi
    cJSON* connectivity = cJSON_GetObjectItem(root, "connectivity");
    if (connectivity) {
        cJSON* wifi = cJSON_GetObjectItem(connectivity, "wifi");
        if (wifi) {
            cJSON* ssid_json = cJSON_GetObjectItem(wifi, "ssid");
            cJSON* password_json = cJSON_GetObjectItem(wifi, "password");
            
            if (ssid_json && password_json && 
                cJSON_IsString(ssid_json) && cJSON_IsString(password_json)) {
                
                esp_err_t err = secure_credentials_set_wifi(ssid_json->valuestring, 
                                                           password_json->valuestring);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set WiFi credentials from config");
                    result = err;
                }
            }
        }
    }
    
    // Load server credentials from data_transmission.server
    cJSON* data_transmission = cJSON_GetObjectItem(root, "data_transmission");
    if (data_transmission) {
        cJSON* server = cJSON_GetObjectItem(data_transmission, "server");
        if (server) {
            cJSON* url_json = cJSON_GetObjectItem(server, "url");
            
            if (url_json && cJSON_IsString(url_json)) {
                esp_err_t err = secure_credentials_set_server(url_json->valuestring, NULL);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set server credentials from config");
                    result = err;
                }
            }
        }
    }
    
    cJSON_Delete(root);
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Credentials loaded from config file successfully");
    }
    
    return result;
} 