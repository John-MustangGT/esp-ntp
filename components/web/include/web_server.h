#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * Start web server on Core 0
 * @param is_ap_mode true if running in AP mode (config portal), false for normal operation
 */
esp_err_t web_server_start(bool is_ap_mode);

/**
 * Stop web server
 */
esp_err_t web_server_stop(void);
