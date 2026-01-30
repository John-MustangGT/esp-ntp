#include "nvs_storage.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "NVS_STORAGE";

#define NVS_CHECKSUM_KEY "checksum"

/**
 * Calculate CRC32 checksum
 */
static uint32_t crc32_checksum(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return crc ^ 0xFFFFFFFF;
}

esp_err_t nvs_storage_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated or new version found, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

bool nvs_config_is_valid(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "NVS namespace not found: %s", esp_err_to_name(err));
        return false;
    }

    // Read stored checksum
    uint32_t stored_checksum = 0;
    err = nvs_get_u32(handle, NVS_CHECKSUM_KEY, &stored_checksum);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Checksum not found in NVS");
        nvs_close(handle);
        return false;
    }

    // Read config
    nvs_config_t config;
    size_t config_len = sizeof(nvs_config_t);
    err = nvs_get_blob(handle, NVS_CONFIG_KEY, &config, &config_len);
    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Config not found in NVS: %s", esp_err_to_name(err));
        return false;
    }

    // Calculate checksum and compare
    uint32_t calculated_checksum = crc32_checksum((uint8_t *)&config, sizeof(nvs_config_t));
    if (calculated_checksum != stored_checksum) {
        ESP_LOGW(TAG, "Checksum mismatch: stored=0x%08x, calculated=0x%08x", 
                 stored_checksum, calculated_checksum);
        return false;
    }

    ESP_LOGI(TAG, "Config validation successful");
    return true;
}

esp_err_t nvs_config_read(nvs_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    size_t config_len = sizeof(nvs_config_t);
    err = nvs_get_blob(handle, NVS_CONFIG_KEY, config, &config_len);
    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read config: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Config read successfully");
    return ESP_OK;
}

esp_err_t nvs_config_write(const nvs_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Write config
    err = nvs_set_blob(handle, NVS_CONFIG_KEY, (void *)config, sizeof(nvs_config_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    // Calculate and write checksum
    uint32_t checksum = crc32_checksum((uint8_t *)config, sizeof(nvs_config_t));
    err = nvs_set_u32(handle, NVS_CHECKSUM_KEY, checksum);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write checksum: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Config written with checksum: 0x%08x", checksum);
    } else {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t nvs_config_erase(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    nvs_erase_key(handle, NVS_CONFIG_KEY);
    nvs_erase_key(handle, NVS_CHECKSUM_KEY);
    err = nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Config erased");
    return err;
}

uint32_t nvs_config_get_checksum(void) {
    nvs_handle_t handle;
    uint32_t checksum = 0;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_u32(handle, NVS_CHECKSUM_KEY, &checksum);
        nvs_close(handle);
    }

    return checksum;
}
