#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_system.h"
#include "esp_timer.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>

static const char* TAG = "HTTP";
static const char* deviceName = "ESP32_Sensor_01";

static char serverUrl[256] = "http://your-server.local:8080/api/data";



static esp_err_t httpEventHandler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP Error");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            break;
        case HTTP_EVENT_HEADER_SENT:
            break; // dont log every header
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGI(TAG, "Response: %.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "Finished");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "Redirect");
            break;
    }
    return ESP_OK;
}

// Consolidated function that handles all sensor data transmission scenarios
bool sendSensorDataToDatabase(const sensorDatabaseData_t *sensorData,
                             uint32_t timeoutMs, 
                             const char* serverUrlParam,
                             uint8_t aqiMin, uint8_t aqiMax,
                             uint16_t tvocMin, uint16_t tvocMax,
                             uint16_t eco2Min, uint16_t eco2Max)
{

    // Use provided URL or fallback to config/default
    const char* targetUrl = serverUrlParam ? serverUrlParam : serverUrl;
    
    // Use provided timeout or default
    uint32_t actualTimeout = timeoutMs > 0 ? timeoutMs : 15000;
    
    // Use provided thresholds or defaults - allow zeros for valid sensor readings
    uint8_t actualAqiMin = aqiMin;  // Allow 0 as valid
    uint8_t actualAqiMax = aqiMax > 0 ? aqiMax : 255;
    uint16_t actualTvocMin = tvocMin;  // Allow 0 as valid
    uint16_t actualTvocMax = tvocMax > 0 ? tvocMax : 65535;
    uint16_t actualEco2Min = eco2Min;  // Allow 0 as valid
    uint16_t actualEco2Max = eco2Max > 0 ? eco2Max : 65535;

    char localResponseBuffer[2048];
    esp_http_client_config_t config = {
        .url = targetUrl,
        .event_handler = httpEventHandler,
        .user_data = localResponseBuffer,
        .timeout_ms = actualTimeout,
        .buffer_size = 2048,
        .buffer_size_tx = 2048,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // Create JSON with all sensor data
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device", deviceName);

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char dateStr[11];
    char timeStr[9];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    
    cJSON_AddStringToObject(root, "currentDate", dateStr);
    cJSON_AddStringToObject(root, "currentTime", timeStr);

    if (sensorData) {
        // Add temperature and humidity data
        if (sensorData->aht21Valid) {
            double tempRounded = round(sensorData->temperature * 100.0) / 100.0;
            double humRounded = round(sensorData->humidity * 100.0) / 100.0;
            
            cJSON_AddNumberToObject(root, "temperature", tempRounded);
            cJSON_AddNumberToObject(root, "humidity", humRounded);
        } else {
            // Send 0.0 instead of null when AHT21 data is not available
            cJSON_AddNumberToObject(root, "temperature", 0.0);
            cJSON_AddNumberToObject(root, "humidity", 0.0);
        }

        // Add ENS160 gas sensor data with configurable thresholds
        if (sensorData->ens160Valid) {
            // Use configurable thresholds for validation
            if (sensorData->aqi >= actualAqiMin && sensorData->aqi <= actualAqiMax &&
                sensorData->tvoc >= actualTvocMin && sensorData->tvoc <= actualTvocMax &&
                sensorData->eco2 >= actualEco2Min && sensorData->eco2 <= actualEco2Max) {
                cJSON_AddNumberToObject(root, "aqi", sensorData->aqi);
                cJSON_AddNumberToObject(root, "tvoc", sensorData->tvoc);
                cJSON_AddNumberToObject(root, "eco2", sensorData->eco2);
                cJSON_AddStringToObject(root, "gasStatus", "valid");
            } else {
                // Values are outside configurable thresholds - log warning but don't add to JSON
                ESP_LOGW(TAG, "ENS160 data outside thresholds - skipping gas data: AQI=%d[%d-%d], TVOC=%d[%d-%d], eCO2=%d[%d-%d]", 
                         sensorData->aqi, actualAqiMin, actualAqiMax,
                         sensorData->tvoc, actualTvocMin, actualTvocMax,
                         sensorData->eco2, actualEco2Min, actualEco2Max);
                // Don't add gas sensor data to JSON - will be omitted from database
                cJSON_AddStringToObject(root, "gasStatus", "skipped");
                cJSON_AddBoolToObject(root, "ens160Valid", false);
            }
        } else {
            // ENS160 data not available - log warning but don't add to JSON
            ESP_LOGW(TAG, "ENS160 data not valid - skipping gas data");
            cJSON_AddStringToObject(root, "gasStatus", "skipped");
            cJSON_AddBoolToObject(root, "ens160Valid", false);
        }

        // Add sensor validity flags
        cJSON_AddBoolToObject(root, "aht21Valid", sensorData->aht21Valid);
        cJSON_AddBoolToObject(root, "ens160Valid", sensorData->ens160Valid);
    } else {
        // Fallback for basic temperature/humidity only mode
        cJSON_AddNumberToObject(root, "temperature", 0.0);
        cJSON_AddNumberToObject(root, "humidity", 0.0);
        cJSON_AddStringToObject(root, "gasStatus", "skipped");
        cJSON_AddBoolToObject(root, "aht21Valid", false);
        cJSON_AddBoolToObject(root, "ens160Valid", false);
    }

    char *postData = cJSON_PrintUnformatted(root);
    ESP_LOGI(TAG, "Sending sensor data (timeout: %lu ms): %s", (unsigned long)actualTimeout, postData);

    esp_http_client_set_post_field(client, postData, strlen(postData));
    esp_err_t err = esp_http_client_perform(client);

    bool success = false;
    if (err == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                statusCode,
                esp_http_client_get_content_length(client));
        
        success = (statusCode >= 200 && statusCode < 300);
        
        if (success) {
            ESP_LOGI(TAG, "Sensor data sent successfully!");
        } else {
            ESP_LOGW(TAG, "Server returned non-success status: %d", statusCode);
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    cJSON_Delete(root);
    free(postData);
    esp_http_client_cleanup(client);
    
    return success;
}



