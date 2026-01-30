#include "pps_handler.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "PPS";

static volatile pps_edge_data_t last_pps_edge = {0};
static volatile bool pps_initialized = false;

// PPS interrupt handler - MUST be fast and lightweight
static void IRAM_ATTR pps_isr_handler(void *arg) {
    // Capture timestamp immediately for precision
    int64_t timestamp = esp_timer_get_time();
    
    // Update PPS data atomically
    last_pps_edge.timestamp_us = timestamp;
    last_pps_edge.count++;
}

esp_err_t pps_init(void) {
    ESP_LOGI(TAG, "Initializing PPS on GPIO%d", PPS_GPIO_PIN);

    // Configure GPIO for PPS input
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,  // Rising edge trigger
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PPS_GPIO_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(err));
        return err;
    }

    // Install GPIO ISR service
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install ISR service: %s", esp_err_to_name(err));
        return err;
    }

    // Attach interrupt handler
    err = gpio_isr_handler_add(PPS_GPIO_PIN, pps_isr_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler: %s", esp_err_to_name(err));
        return err;
    }

    pps_initialized = true;
    ESP_LOGI(TAG, "PPS handler initialized successfully");
    
    return ESP_OK;
}

bool pps_get_last_edge(pps_edge_data_t *data) {
    if (!pps_initialized || data == NULL) {
        return false;
    }

    // Read atomically
    data->timestamp_us = last_pps_edge.timestamp_us;
    data->count = last_pps_edge.count;
    
    return (data->count > 0);
}

uint32_t pps_get_pulse_count(void) {
    return last_pps_edge.count;
}

bool pps_is_active(uint32_t timeout_ms) {
    if (!pps_initialized || last_pps_edge.count == 0) {
        return false;
    }

    int64_t now = esp_timer_get_time();
    int64_t elapsed_us = now - last_pps_edge.timestamp_us;
    
    return (elapsed_us < (timeout_ms * 1000));
}

void pps_reset(void) {
    last_pps_edge.timestamp_us = 0;
    last_pps_edge.count = 0;
    ESP_LOGI(TAG, "PPS statistics reset");
}
