#include "time_sync.h"
#include "ntp_client.h"
#include "gps_nmea.h"
#include "pps_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TIME_SYNC";

static time_source_t current_source = TIME_SOURCE_NONE;
static bool is_synced = false;

esp_err_t time_sync_init(void) {
    ESP_LOGI(TAG, "Time sync orchestrator initialized");
    return ESP_OK;
}

static void time_sync_task(void *pvParameters) {
    ESP_LOGI(TAG, "Time sync task started");
    
    while (1) {
        // Priority: GPS+PPS > GPS > NTP
        
        if (gps_has_fix() && pps_is_active(2000)) {
            // Best case: GPS time with PPS discipline
            current_source = TIME_SOURCE_GPS_PPS;
            is_synced = true;
            ESP_LOGD(TAG, "Time source: GPS+PPS");
            
        } else if (gps_has_fix()) {
            // GPS time without PPS
            current_source = TIME_SOURCE_GPS;
            is_synced = true;
            ESP_LOGD(TAG, "Time source: GPS");
            
        } else if (ntp_is_synced()) {
            // Fallback to NTP
            current_source = TIME_SOURCE_NTP;
            is_synced = true;
            ESP_LOGD(TAG, "Time source: NTP");
            
            // Try to refresh NTP every 10 minutes
            if (ntp_get_seconds_since_sync() > 600) {
                ESP_LOGI(TAG, "Refreshing NTP time...");
                ntp_sync_system_time();
            }
        } else {
            // No time source available
            current_source = TIME_SOURCE_NONE;
            is_synced = false;
            ESP_LOGW(TAG, "No time source available");
            
            // Try NTP sync
            ESP_LOGI(TAG, "Attempting NTP sync...");
            ntp_sync_system_time();
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Check every 5 seconds
    }
}

esp_err_t time_sync_start(void) {
    xTaskCreatePinnedToCore(time_sync_task, "time_sync_task", 4096, NULL, 5, NULL, 0);
    ESP_LOGI(TAG, "Time sync task started on Core 0");
    return ESP_OK;
}

time_source_t time_sync_get_source(void) {
    return current_source;
}

bool time_sync_is_synced(void) {
    return is_synced;
}
