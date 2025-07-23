/*
 * Mock WiFi Functions for ESP32 Environmental Monitor Tests
 * 
 * This header defines mock functions for WiFi operations used by network components.
 * These mocks allow testing network functionality without real WiFi hardware.
 */

#ifndef MOCK_WIFI_FUNCTIONS_H
#define MOCK_WIFI_FUNCTIONS_H

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <stdint.h>
#include <stdbool.h>

// Mock WiFi initialisation
esp_err_t mockEspWifiInit(const wifi_init_config_t* config);

// Mock WiFi mode setting
esp_err_t mockEspWifiSetMode(wifi_mode_t mode);

// Mock WiFi station configuration
esp_err_t mockEspWifiSetConfig(wifi_interface_t interface, wifi_config_t* conf);

// Mock WiFi start
esp_err_t mockEspWifiStart(void);

// Mock WiFi stop
esp_err_t mockEspWifiStop(void);

// Mock WiFi connect
esp_err_t mockEspWifiConnect(void);

// Mock WiFi disconnect
esp_err_t mockEspWifiDisconnect(void);

// Mock WiFi status getter
wifi_ap_record_t* mockEspWifiGetApInfo(void);

// Mock WiFi signal strength
int8_t mockEspWifiGetRssi(void);

// Mock WiFi connection status
bool mockEspWifiIsConnected(void);

// Mock WiFi event handling
esp_err_t mockEspEventLoopCreateDefault(void);
esp_err_t mockEspEventHandlerRegister(esp_event_base_t eventBase, int32_t eventId, 
                                     esp_event_handler_t eventHandler, void* eventHandlerArg);

// Mock control functions for test setup
void mockWifiSetConnected(bool connected);
void mockWifiSetSignalStrength(int8_t rssi);
void mockWifiSetApInfo(const wifi_ap_record_t* apInfo);
void mockWifiResetState(void);

#endif // MOCK_WIFI_FUNCTIONS_H 