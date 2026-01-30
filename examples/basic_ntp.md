# Example: Basic NTP Time Sync

This example demonstrates basic NTP time synchronization without GPS.

## Usage

```c
#include "ntp_client.h"
#include "esp_log.h"

void app_main(void) {
    // Initialize WiFi first...
    
    // Initialize NTP client
    ntp_client_init("pool.ntp.org");
    
    // Sync system time
    esp_err_t err = ntp_sync_system_time();
    if (err == ESP_OK) {
        ESP_LOGI("NTP", "Time synchronized successfully");
        
        // Print current time
        time_t now;
        struct tm timeinfo;
        char strftime_buf[64];
        
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        
        ESP_LOGI("NTP", "Current time: %s", strftime_buf);
    }
}
```

## Configuration

- Default NTP server: `pool.ntp.org`
- Default timeout: 5000ms
- Port: 123 (standard NTP)
