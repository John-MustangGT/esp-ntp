#include "oled_display.h"
#include "esp_log.h"

static const char *TAG = "OLED";

// TODO: Implement with U8g2 or SSD1306 library

esp_err_t oled_init(void) {
    ESP_LOGI(TAG, "OLED display initialization (stub)");
    // TODO: Initialize I2C and OLED display
    return ESP_OK;
}

esp_err_t oled_clear(void) {
    ESP_LOGD(TAG, "Clear display (stub)");
    return ESP_OK;
}

esp_err_t oled_display_status(const char *line1, const char *line2, const char *line3) {
    ESP_LOGD(TAG, "Display: %s | %s | %s", line1, line2, line3);
    // TODO: Render to OLED
    return ESP_OK;
}

esp_err_t oled_display_time(const char *time_str) {
    ESP_LOGD(TAG, "Display time: %s", time_str);
    // TODO: Render time to OLED
    return ESP_OK;
}
