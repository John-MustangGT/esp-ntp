#pragma once

#include "esp_err.h"
#include <time.h>
#include <stdbool.h>

#define GPS_UART_NUM UART_NUM_1
#define GPS_UART_BAUD 9600
#define GPS_UART_RX_PIN 44  // Default for XIAO ESP32-S3
#define GPS_UART_TX_PIN -1  // Not used for receive-only

/**
 * GPS data structure
 */
typedef struct {
    bool valid;
    struct tm time;
    double latitude;
    double longitude;
    float altitude;
    uint8_t satellites;
    float hdop;
} gps_data_t;

/**
 * Initialize GPS UART communication
 */
esp_err_t gps_init(void);

/**
 * Parse incoming GPS data (call periodically)
 * @param data Pointer to store parsed GPS data
 * @return ESP_OK if valid data parsed
 */
esp_err_t gps_update(gps_data_t *data);

/**
 * Check if GPS has valid fix
 */
bool gps_has_fix(void);

/**
 * Get GPS time as Unix timestamp
 */
time_t gps_get_unix_time(void);

/**
 * Start GPS task on Core 1
 */
esp_err_t gps_start_task(void);
