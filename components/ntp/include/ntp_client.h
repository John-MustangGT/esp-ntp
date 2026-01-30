#pragma once

#include "esp_err.h"
#include <time.h>
#include <stdbool.h>

#define NTP_DEFAULT_SERVER "pool.ntp.org"
#define NTP_PORT 123
#define NTP_TIMEOUT_MS 5000

/**
 * NTP packet structure (48 bytes)
 */
typedef struct {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_timestamp_sec;
    uint32_t ref_timestamp_frac;
    uint32_t orig_timestamp_sec;
    uint32_t orig_timestamp_frac;
    uint32_t recv_timestamp_sec;
    uint32_t recv_timestamp_frac;
    uint32_t trans_timestamp_sec;
    uint32_t trans_timestamp_frac;
} ntp_packet_t;

/**
 * Initialize NTP client
 */
esp_err_t ntp_client_init(const char *server);

/**
 * Perform NTP sync and get time
 * @param time_out Pointer to store the synchronized time
 * @return ESP_OK on success
 */
esp_err_t ntp_sync(struct timeval *time_out);

/**
 * Set system time from NTP
 * @return ESP_OK on success
 */
esp_err_t ntp_sync_system_time(void);

/**
 * Get last sync status
 * @return true if last sync was successful
 */
bool ntp_is_synced(void);

/**
 * Get time since last successful sync (seconds)
 */
uint32_t ntp_get_seconds_since_sync(void);
