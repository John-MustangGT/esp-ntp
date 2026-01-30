#include "gps_nmea.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "GPS";

#define GPS_RX_BUF_SIZE 1024
#define NMEA_MAX_LENGTH 256

static gps_data_t current_gps_data = {0};
static bool gps_initialized = false;

esp_err_t gps_init(void) {
    uart_config_t uart_config = {
        .baud_rate = GPS_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_UART_TX_PIN, GPS_UART_RX_PIN, 
                                   UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, GPS_RX_BUF_SIZE, 0, 0, NULL, 0));

    gps_initialized = true;
    ESP_LOGI(TAG, "GPS UART initialized on UART%d (RX: GPIO%d, Baud: %d)", 
             GPS_UART_NUM, GPS_UART_RX_PIN, GPS_UART_BAUD);
    
    return ESP_OK;
}

// Calculate NMEA checksum
static uint8_t nmea_checksum(const char *sentence) {
    uint8_t checksum = 0;
    const char *p = sentence;
    
    if (*p == '$') p++;  // Skip '$'
    
    while (*p && *p != '*') {
        checksum ^= *p;
        p++;
    }
    
    return checksum;
}

// Parse GPRMC sentence: $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
static bool parse_gprmc(const char *sentence, gps_data_t *data) {
    char *token;
    char *saveptr;
    char *sentence_copy = strdup(sentence);
    int field = 0;
    
    char time_str[16] = {0};
    char date_str[16] = {0};
    char lat_str[32] = {0};
    char lat_dir = 'N';
    char lon_str[32] = {0};
    char lon_dir = 'E';
    char status = 'V';
    
    token = strtok_r(sentence_copy, ",", &saveptr);
    while (token != NULL) {
        switch (field) {
            case 1: strncpy(time_str, token, sizeof(time_str) - 1); break;
            case 2: status = token[0]; break;
            case 3: strncpy(lat_str, token, sizeof(lat_str) - 1); break;
            case 4: lat_dir = token[0]; break;
            case 5: strncpy(lon_str, token, sizeof(lon_str) - 1); break;
            case 6: lon_dir = token[0]; break;
            case 9: strncpy(date_str, token, sizeof(date_str) - 1); break;
        }
        token = strtok_r(NULL, ",", &saveptr);
        field++;
    }
    
    free(sentence_copy);
    
    if (status != 'A') {
        data->valid = false;
        return false;
    }
    
    // Parse time: hhmmss.ss
    if (strlen(time_str) >= 6) {
        data->time.tm_hour = (time_str[0] - '0') * 10 + (time_str[1] - '0');
        data->time.tm_min = (time_str[2] - '0') * 10 + (time_str[3] - '0');
        data->time.tm_sec = (time_str[4] - '0') * 10 + (time_str[5] - '0');
    }
    
    // Parse date: ddmmyy
    if (strlen(date_str) >= 6) {
        data->time.tm_mday = (date_str[0] - '0') * 10 + (date_str[1] - '0');
        data->time.tm_mon = ((date_str[2] - '0') * 10 + (date_str[3] - '0')) - 1;
        data->time.tm_year = ((date_str[4] - '0') * 10 + (date_str[5] - '0')) + 100;  // Years since 1900
    }
    
    // Parse latitude: ddmm.mmmm
    if (strlen(lat_str) > 0) {
        double lat = atof(lat_str);
        int deg = (int)(lat / 100.0);
        double min = lat - (deg * 100.0);
        data->latitude = deg + (min / 60.0);
        if (lat_dir == 'S') data->latitude = -data->latitude;
    }
    
    // Parse longitude: dddmm.mmmm
    if (strlen(lon_str) > 0) {
        double lon = atof(lon_str);
        int deg = (int)(lon / 100.0);
        double min = lon - (deg * 100.0);
        data->longitude = deg + (min / 60.0);
        if (lon_dir == 'W') data->longitude = -data->longitude;
    }
    
    data->valid = true;
    return true;
}

// Parse GPGGA sentence for altitude and satellite count
static bool parse_gpgga(const char *sentence, gps_data_t *data) {
    char *token;
    char *saveptr;
    char *sentence_copy = strdup(sentence);
    int field = 0;
    
    token = strtok_r(sentence_copy, ",", &saveptr);
    while (token != NULL) {
        switch (field) {
            case 7: data->satellites = atoi(token); break;
            case 8: data->hdop = atof(token); break;
            case 9: data->altitude = atof(token); break;
        }
        token = strtok_r(NULL, ",", &saveptr);
        field++;
    }
    
    free(sentence_copy);
    return true;
}

esp_err_t gps_update(gps_data_t *data) {
    if (!gps_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buffer[NMEA_MAX_LENGTH];
    int len = uart_read_bytes(GPS_UART_NUM, buffer, sizeof(buffer) - 1, pdMS_TO_TICKS(100));
    
    if (len > 0) {
        buffer[len] = '\0';
        
        // Process each line
        char *line = strtok((char *)buffer, "\r\n");
        while (line != NULL) {
            if (line[0] == '$' && strlen(line) > 10) {
                // Verify checksum
                char *checksum_ptr = strchr(line, '*');
                if (checksum_ptr) {
                    uint8_t expected = nmea_checksum(line);
                    uint8_t received = strtol(checksum_ptr + 1, NULL, 16);
                    
                    if (expected == received) {
                        if (strstr(line, "$GPRMC") || strstr(line, "$GNRMC")) {
                            parse_gprmc(line, &current_gps_data);
                        } else if (strstr(line, "$GPGGA") || strstr(line, "$GNGGA")) {
                            parse_gpgga(line, &current_gps_data);
                        }
                    }
                }
            }
            line = strtok(NULL, "\r\n");
        }
    }
    
    if (data) {
        memcpy(data, &current_gps_data, sizeof(gps_data_t));
    }
    
    return ESP_OK;
}

bool gps_has_fix(void) {
    return current_gps_data.valid;
}

time_t gps_get_unix_time(void) {
    if (!current_gps_data.valid) {
        return 0;
    }
    return mktime(&current_gps_data.time);
}

static void gps_task(void *pvParameters) {
    ESP_LOGI(TAG, "GPS task running on Core 1");
    
    gps_data_t data;
    while (1) {
        gps_update(&data);
        
        if (data.valid) {
            ESP_LOGD(TAG, "GPS Fix: Lat=%.6f, Lon=%.6f, Alt=%.1fm, Sats=%d",
                     data.latitude, data.longitude, data.altitude, data.satellites);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t gps_start_task(void) {
    xTaskCreatePinnedToCore(gps_task, "gps_task", 4096, NULL, 5, NULL, 1);
    ESP_LOGI(TAG, "GPS task started on Core 1");
    return ESP_OK;
}
