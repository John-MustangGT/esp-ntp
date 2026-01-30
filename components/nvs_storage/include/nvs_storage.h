#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define NVS_CONFIG_KEY "config"
#define NVS_NAMESPACE "nvs_config"
#define NVS_CONFIG_VERSION 1

/**
 * Configuration structure stored in NVS
 */
typedef struct {
    uint8_t version;
    char ssid[32];
    char password[64];
    char ntp_server[128];
    int32_t timezone_offset_sec;
    uint8_t reserved[16];
} nvs_config_t;

/**
 * Initialize NVS storage
 */
esp_err_t nvs_storage_init(void);

/**
 * Validate NVS configuration with checksum
 * @return true if config is valid, false otherwise
 */
bool nvs_config_is_valid(void);

/**
 * Read configuration from NVS
 * @param config pointer to config structure
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if not present
 */
esp_err_t nvs_config_read(nvs_config_t *config);

/**
 * Write configuration to NVS with checksum
 * @param config pointer to config structure
 * @return ESP_OK on success
 */
esp_err_t nvs_config_write(const nvs_config_t *config);

/**
 * Erase configuration from NVS
 * @return ESP_OK on success
 */
esp_err_t nvs_config_erase(void);

/**
 * Get stored checksum for validation
 * @return checksum value
 */
uint32_t nvs_config_get_checksum(void);
