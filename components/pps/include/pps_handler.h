#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#define PPS_GPIO_PIN 1  // GPIO1 for PPS input

/**
 * PPS edge data structure
 */
typedef struct {
    int64_t timestamp_us;   // Microsecond timestamp from esp_timer
    uint32_t count;         // Pulse count since initialization
} pps_edge_data_t;

/**
 * Initialize PPS interrupt handler on GPIO1
 */
esp_err_t pps_init(void);

/**
 * Get last PPS edge timestamp
 * @param data Pointer to store PPS edge data
 * @return true if data is available
 */
bool pps_get_last_edge(pps_edge_data_t *data);

/**
 * Get PPS pulse count
 */
uint32_t pps_get_pulse_count(void);

/**
 * Check if PPS signal is active (pulse received recently)
 * @param timeout_ms Maximum time since last pulse
 * @return true if pulse received within timeout
 */
bool pps_is_active(uint32_t timeout_ms);

/**
 * Reset PPS statistics
 */
void pps_reset(void);
