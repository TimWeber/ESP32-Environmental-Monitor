#include "http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include <string.h>
#include <sys/socket.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "HTTP_SERVER";

// Health callback function
static const char* (*health_callback)(const char*) = NULL;

// HTTP server task handle
static TaskHandle_t http_server_task = NULL;
static bool server_running = false;

// Simple HTTP response builder
static void send_http_response(int sock, int status_code, const char* content_type, const char* body) {
    char response[4096]; // Increased buffer size
    int written = snprintf(response, sizeof(response),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status_code, content_type, strlen(body), body);
    
    if (written >= 0 && written < sizeof(response)) {
        int sent = send(sock, response, written, 0);
        if (sent < 0) {
            ESP_LOGE(TAG, "Failed to send response: errno %d", errno);
        } else {
            ESP_LOGI(TAG, "Response sent successfully (%d bytes)", sent);
        }
    } else {
        ESP_LOGE(TAG, "Response buffer overflow");
    }
}

// Handle HTTP request
static void handle_http_request(int sock, const char* request) {
    ESP_LOGI(TAG, "HTTP request received: %s", request);
    
    // Check if it's a GET request to /api/health
    if (strstr(request, "GET /api/health") != NULL) {
        ESP_LOGI(TAG, "Health endpoint requested");
        
        if (health_callback) {
            const char* response = health_callback(request);
            if (response) {
                send_http_response(sock, 200, "application/json", response);
                return;
            }
        }
        
        // Default health response
        const char* default_response = "{\"error\": \"Health monitoring not available\"}";
        send_http_response(sock, 200, "application/json", default_response);
        return;
    }
    
    // Check if it's a GET request to root
    if (strstr(request, "GET /") != NULL) {
        const char* response = "ESP32 Environmental Monitor\n"
                              "Health endpoint: GET /api/health\n"
                              "Data endpoint: POST /api/data";
        send_http_response(sock, 200, "text/plain", response);
        return;
    }
    
    // 404 for unknown endpoints
    send_http_response(sock, 404, "text/plain", "Not Found");
}

// HTTP server task
static void http_server_task_func(void* pvParameters) {
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSE, &opt, sizeof(opt));
    
    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(HTTP_SERVER_PORT);
    
    int err = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(server_sock);
        vTaskDelete(NULL);
        return;
    }
    
    err = listen(server_sock, 5); // Increased backlog
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(server_sock);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "HTTP server listening on port %d", HTTP_SERVER_PORT);
    
    while (server_running) {
        struct sockaddr_in source_addr;
        socklen_t len = sizeof(source_addr);
        int sock = accept(server_sock, (struct sockaddr*)&source_addr, &len);
        if (sock < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            }
            vTaskDelay(pdMS_TO_TICKS(100)); // Add delay to prevent busy loop
            continue;
        }
        
        ESP_LOGI(TAG, "Socket accepted from %s", inet_ntoa(source_addr.sin_addr));
        
        // Read HTTP request with timeout
        char buffer[2048];
        int len_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (len_received > 0) {
            buffer[len_received] = '\0';
            handle_http_request(sock, buffer);
        } else if (len_received == 0) {
            ESP_LOGW(TAG, "Client disconnected before sending request");
        } else {
            ESP_LOGE(TAG, "Failed to receive request: errno %d", errno);
        }
        
        // Properly close the connection
        shutdown(sock, SHUT_RDWR);
        close(sock);
        ESP_LOGI(TAG, "Connection closed");
    }
    
    close(server_sock);
    ESP_LOGI(TAG, "HTTP server task ending");
    vTaskDelete(NULL);
}

esp_err_t http_server_init(void) {
    ESP_LOGI(TAG, "Initialising HTTP server");
    
    // Start HTTP server task
    server_running = true;
    BaseType_t ret = xTaskCreate(http_server_task_func, "http_server", 8192, NULL, 5, &http_server_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create HTTP server task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "HTTP server initialised successfully");
    return ESP_OK;
}

void http_server_deinit(void) {
    ESP_LOGI(TAG, "Deinitialising HTTP server");
    
    server_running = false;
    
    if (http_server_task) {
        vTaskDelete(http_server_task);
        http_server_task = NULL;
    }
    
    ESP_LOGI(TAG, "HTTP server deinitialised");
}

esp_err_t http_server_start(void) {
    // Server is started during init
    return ESP_OK;
}

void http_server_stop(void) {
    http_server_deinit();
}

esp_err_t http_server_register_health_callback(const char* (*callback)(const char* request_data)) {
    health_callback = callback;
    ESP_LOGI(TAG, "Health callback registered");
    return ESP_OK;
} 