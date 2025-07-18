#include "sntp_manager.h"
#include "esp_log.h"
#include "esp_sntp.h"

static const char* TAG = "SNTP";

void sntp_manager_init(void)
{
    ESP_LOGI(TAG, "Initialising SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");  // using default NTP pool
    esp_sntp_init();
}

