/**
 * @file oled_display.h
 * @brief OLED display driver for 128x32 I2C OLED
 * 
 * Configuration:
 * - I2C: GPIO 5 (SDA), GPIO 6 (SCL)
 */

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize OLED display
 */
void oled_init(void);

/**
 * @brief Clear display
 */
void oled_clear(void);

/**
 * @brief Display text on a specific line
 * @param line Line number (0-3 for 128x32 display)
 * @param text Text to display
 * @param invert Invert display (1 = inverted, 0 = normal)
 */
void oled_display_text(uint8_t line, const char *text, uint8_t invert);

#ifdef __cplusplus
}
#endif

#endif // OLED_DISPLAY_H
