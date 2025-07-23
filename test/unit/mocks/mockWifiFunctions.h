/*
 * Mock WiFi Functions for ESP32 Environmental Monitor Tests
 * 
 * This header defines mock functions for WiFi operations used by the network module.
 * These mocks allow testing network functionality without real WiFi hardware.
 */

#ifndef MOCK_WIFI_FUNCTIONS_H
#define MOCK_WIFI_FUNCTIONS_H

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <stdint.h>
#include <stddef.h>

// Mock initialization and cleanup
void mockWifiFunctionsInit(void);
void mockWifiFunctionsCleanup(void);

// Mock WiFi initialization
esp_err_t mockEspWifiInit(const wifi_init_config_t* config);

// Mock WiFi mode setting
esp_err_t mockEspWifiSetMode(wifi_mode_t mode);

// Mock WiFi configuration
esp_err_t mockEspWifiSetConfig(wifi_interface_t interface, wifi_config_t* conf);

// Mock WiFi start
esp_err_t mockEspWifiStart(void);

// Mock WiFi stop
esp_err_t mockEspWifiStop(void);

// Mock WiFi connect
esp_err_t mockEspWifiConnect(void);

// Mock WiFi disconnect
esp_err_t mockEspWifiDisconnect(void);

// Mock WiFi scan
esp_err_t mockEspWifiScanStart(const wifi_scan_config_t* config, bool block);

// Mock WiFi scan stop
esp_err_t mockEspWifiScanStop(void);

// Mock WiFi scan get results
esp_err_t mockEspWifiScanGetApNum(uint16_t* number);

// Mock WiFi scan get results
esp_err_t mockEspWifiScanGetApRecords(uint16_t* number, wifi_ap_record_t* ap_records);

// Mock WiFi get status
wifi_ap_record_t* mockEspWifiScanGetApRecords(void);

// Mock WiFi get MAC address
esp_err_t mockEspWifiGetMac(wifi_interface_t ifx, uint8_t mac[6]);

// Mock WiFi get IP address
esp_err_t mockEspWifiGetIpInfo(wifi_interface_t ifx, esp_netif_ip_info_t* info);

// Mock WiFi event handler registration
esp_err_t mockEspEventHandlerRegister(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void* event_arg);

// Mock WiFi event handler unregistration
esp_err_t mockEspEventHandlerUnregister(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler);

// Mock WiFi event posting
esp_err_t mockEspEventPost(esp_event_base_t event_base, int32_t event_id, void* event_data, size_t event_data_size, TickType_t timeout);

#endif // MOCK_WIFI_FUNCTIONS_H 