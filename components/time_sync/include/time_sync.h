#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
    TIME_SOURCE_NONE = 0,
    TIME_SOURCE_NTP,
    TIME_SOURCE_GPS,
    TIME_SOURCE_GPS_PPS
} time_source_t;

/**
 * Initialize time synchronization orchestrator
 */
esp_err_t time_sync_init(void);

/**
 * Start time sync task
 */
esp_err_t time_sync_start(void);

/**
 * Get current time source
 */
time_source_t time_sync_get_source(void);

/**
 * Check if time is synchronized
 */
bool time_sync_is_synced(void);
