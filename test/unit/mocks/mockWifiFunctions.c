/*
 * Mock WiFi Functions Implementation for ESP32 Environmental Monitor Tests
 * 
 * This file provides mock implementations of WiFi functions for unit testing.
 * These mocks simulate WiFi hardware responses without requiring real network hardware.
 */

#include "mockWifiFunctions.h"
#include "esp_log.h"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "MOCK_WIFI";

// Mock WiFi state
static bool mockWifiConnected = false;
static int8_t mockWifiRssi = -45; // Default good signal strength
static wifi_ap_record_t mockApInfo = {
    .ssid = "MockWiFiNetwork",
    .bssid = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},
    .channel = 6,
    .rssi = -45,
    .authmode = WIFI_AUTH_WPA2_PSK,
    .pairwise_cipher = WIFI_CIPHER_TYPE_CCMP,
    .group_cipher = WIFI_CIPHER_TYPE_CCMP,
    .ant = WIFI_ANT_AUTO,
    .phy = WIFI_PHY_11BGN,
    .country = {0x00, 0x00, 0x00},
    .second = 0,
    .reserved = 0
};

// Mock initialization and cleanup
void mockWifiFunctionsInit(void) {
    ESP_LOGI(TAG, "Initializing mock WiFi functions");
    mockWifiConnected = false;
    mockWifiRssi = -45;
}

void mockWifiFunctionsCleanup(void) {
    ESP_LOGI(TAG, "Cleaning up mock WiFi functions");
    mockWifiConnected = false;
}

// Mock WiFi initialisation
esp_err_t mockEspWifiInit(const wifi_init_config_t* config) {
    ESP_LOGI(TAG, "Mock WiFi initialisation called");
    
    if (config == NULL) {
        ESP_LOGE(TAG, "Mock WiFi config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Mock WiFi initialised successfully");
    return ESP_OK;
}

// Mock WiFi mode setting
esp_err_t mockEspWifiSetMode(wifi_mode_t mode) {
    ESP_LOGI(TAG, "Mock WiFi set mode called: %d", mode);
    
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_APSTA) {
        ESP_LOGE(TAG, "Mock WiFi invalid mode: %d", mode);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Mock WiFi mode set successfully");
    return ESP_OK;
}

// Mock WiFi station configuration
esp_err_t mockEspWifiSetConfig(wifi_interface_t interface, wifi_config_t* conf) {
    ESP_LOGI(TAG, "Mock WiFi set config called for interface: %d", interface);
    
    if (conf == NULL) {
        ESP_LOGE(TAG, "Mock WiFi config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (interface != WIFI_IF_STA) {
        ESP_LOGE(TAG, "Mock WiFi invalid interface: %d", interface);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Mock WiFi configured for SSID: %s", conf->sta.ssid);
    return ESP_OK;
}

// Mock WiFi start
esp_err_t mockEspWifiStart(void) {
    ESP_LOGI(TAG, "Mock WiFi start called");
    ESP_LOGI(TAG, "Mock WiFi started successfully");
    return ESP_OK;
}

// Mock WiFi stop
esp_err_t mockEspWifiStop(void) {
    ESP_LOGI(TAG, "Mock WiFi stop called");
    ESP_LOGI(TAG, "Mock WiFi stopped successfully");
    return ESP_OK;
}

// Mock WiFi connect
esp_err_t mockEspWifiConnect(void) {
    ESP_LOGI(TAG, "Mock WiFi connect called");
    
    if (mockWifiConnected) {
        ESP_LOGW(TAG, "Mock WiFi already connected");
        return ESP_ERR_INVALID_STATE;
    }
    
    mockWifiConnected = true;
    ESP_LOGI(TAG, "Mock WiFi connected successfully");
    return ESP_OK;
}

// Mock WiFi disconnect
esp_err_t mockEspWifiDisconnect(void) {
    ESP_LOGI(TAG, "Mock WiFi disconnect called");
    
    if (!mockWifiConnected) {
        ESP_LOGW(TAG, "Mock WiFi not connected");
        return ESP_ERR_INVALID_STATE;
    }
    
    mockWifiConnected = false;
    ESP_LOGI(TAG, "Mock WiFi disconnected successfully");
    return ESP_OK;
}

// Mock WiFi status getter
wifi_ap_record_t* mockEspWifiGetApInfo(void) {
    ESP_LOGI(TAG, "Mock WiFi get AP info called");
    
    if (!mockWifiConnected) {
        ESP_LOGW(TAG, "Mock WiFi not connected, no AP info available");
        return NULL;
    }
    
    // Update RSSI in AP info
    mockApInfo.rssi = mockWifiRssi;
    
    ESP_LOGI(TAG, "Mock WiFi AP info returned for SSID: %s", mockApInfo.ssid);
    return &mockApInfo;
}

// Mock WiFi signal strength
int8_t mockEspWifiGetRssi(void) {
    ESP_LOGI(TAG, "Mock WiFi get RSSI called");
    
    if (!mockWifiConnected) {
        ESP_LOGW(TAG, "Mock WiFi not connected, RSSI unavailable");
        return -100; // Very weak signal
    }
    
    ESP_LOGI(TAG, "Mock WiFi RSSI returned: %d", mockWifiRssi);
    return mockWifiRssi;
}

// Mock WiFi connection status
bool mockEspWifiIsConnected(void) {
    ESP_LOGI(TAG, "Mock WiFi is connected check called");
    ESP_LOGI(TAG, "Mock WiFi connection status: %s", mockWifiConnected ? "connected" : "disconnected");
    return mockWifiConnected;
}

// Mock WiFi event handling
esp_err_t mockEspEventLoopCreateDefault(void) {
    ESP_LOGI(TAG, "Mock WiFi event loop creation called");
    ESP_LOGI(TAG, "Mock WiFi event loop created successfully");
    return ESP_OK;
}

esp_err_t mockEspEventHandlerRegister(esp_event_base_t eventBase, int32_t eventId, 
                                     esp_event_handler_t eventHandler, void* eventHandlerArg) {
    ESP_LOGI(TAG, "Mock WiFi event handler registration called");
    
    if (eventHandler == NULL) {
        ESP_LOGE(TAG, "Mock WiFi event handler is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Mock WiFi event handler registered successfully");
    return ESP_OK;
}

// Mock control functions for test setup
void mockWifiSetConnected(bool connected) {
    mockWifiConnected = connected;
    ESP_LOGI(TAG, "Mock WiFi: Connection status set to %s", connected ? "connected" : "disconnected");
}

void mockWifiSetSignalStrength(int8_t rssi) {
    mockWifiRssi = rssi;
    ESP_LOGI(TAG, "Mock WiFi: Signal strength set to %d", rssi);
}

void mockWifiSetApInfo(const wifi_ap_record_t* apInfo) {
    if (apInfo != NULL) {
        memcpy(&mockApInfo, apInfo, sizeof(wifi_ap_record_t));
        ESP_LOGI(TAG, "Mock WiFi: AP info updated for SSID: %s", mockApInfo.ssid);
    }
}

void mockWifiResetState(void) {
    mockWifiConnected = false;
    mockWifiRssi = -45;
    memset(&mockApInfo, 0, sizeof(wifi_ap_record_t));
    strcpy((char*)mockApInfo.ssid, "MockWiFiNetwork");
    mockApInfo.rssi = -45;
    mockApInfo.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_LOGI(TAG, "Mock WiFi: All state reset to defaults");
} 