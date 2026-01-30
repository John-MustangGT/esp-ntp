#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#define WIFI_AP_SSID "ESP-NTP-Config"
#define WIFI_AP_PASSWORD "configure"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONN 4

/**
 * Initialize WiFi in AP mode for configuration
 */
esp_err_t wifi_start_ap_mode(void);

/**
 * Initialize WiFi in station mode with stored credentials
 */
esp_err_t wifi_start_station_mode(const char *ssid, const char *password);

/**
 * Stop WiFi and clean up
 */
esp_err_t wifi_stop(void);

/**
 * Check if WiFi is connected
 */
bool wifi_is_connected(void);

/**
 * Wait for WiFi connection (blocking, with timeout)
 * @param timeout_ms Maximum time to wait in milliseconds
 * @return true if connected, false on timeout
 */
bool wifi_wait_for_connection(uint32_t timeout_ms);
