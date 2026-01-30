#pragma once

#include "esp_err.h"

#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

/**
 * Initialize OLED display on I2C
 */
esp_err_t oled_init(void);

/**
 * Clear display
 */
esp_err_t oled_clear(void);

/**
 * Display status text
 */
esp_err_t oled_display_status(const char *line1, const char *line2, const char *line3);

/**
 * Display time
 */
esp_err_t oled_display_time(const char *time_str);
