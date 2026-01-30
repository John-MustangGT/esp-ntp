/**
 * @file gps_driver.h
 * @brief GPS driver for ATGM336M GPS module
 * 
 * Configuration:
 * - UART: GPIO 43 (TX), GPIO 44 (RX)
 * - PPS: GPIO 1
 */

#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int hour;
    int minute;
    int second;
    int day;
    int month;
    int year;
    double latitude;
    double longitude;
    double altitude;
    int satellites;
    bool fix_valid;
    uint64_t pps_timestamp;  // Microseconds since boot at last PPS
} gps_data_t;

/**
 * @brief Initialize GPS driver
 */
void gps_init(void);

/**
 * @brief Get current GPS data
 * @param data Pointer to gps_data_t structure to fill
 * @return true if data is available, false otherwise
 */
bool gps_get_data(gps_data_t *data);

/**
 * @brief Get time of last PPS pulse
 * @return Timestamp in microseconds since boot
 */
uint64_t gps_get_pps_timestamp(void);

/**
 * @brief Check if GPS has valid fix
 * @return true if GPS has valid fix, false otherwise
 */
bool gps_has_fix(void);

#ifdef __cplusplus
}
#endif

#endif // GPS_DRIVER_H
