/**
 * @file gps_driver.c
 * @brief GPS driver implementation for ATGM336M GPS module
 */

#include "gps_driver.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "GPS";

// UART configuration
#define GPS_UART_NUM        UART_NUM_1
#define GPS_TX_PIN          43
#define GPS_RX_PIN          44
#define GPS_UART_BAUD_RATE  9600
#define GPS_BUF_SIZE        1024

// PPS configuration
#define GPS_PPS_PIN         1

// Global GPS data
static gps_data_t g_gps_data = {0};
static bool g_data_valid = false;
static uint64_t g_pps_timestamp = 0;

// NMEA parser state
static char nmea_buffer[256];
static int nmea_buffer_pos = 0;

/**
 * @brief PPS interrupt handler
 */
static void IRAM_ATTR pps_isr_handler(void* arg)
{
    g_pps_timestamp = esp_timer_get_time();
}

/**
 * @brief Parse NMEA checksum
 */
static bool nmea_verify_checksum(const char *sentence)
{
    if (sentence[0] != '$') return false;
    
    const char *checksum_str = strchr(sentence, '*');
    if (!checksum_str) return false;
    
    uint8_t checksum = 0;
    for (const char *p = sentence + 1; p < checksum_str; p++) {
        checksum ^= *p;
    }
    
    uint8_t expected = (uint8_t)strtol(checksum_str + 1, NULL, 16);
    return checksum == expected;
}

/**
 * @brief Parse GPGGA sentence (GPS Fix Data)
 */
static void parse_gpgga(const char *sentence)
{
    char *token;
    char *saveptr;
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    int field = 0;
    token = strtok_r(buffer, ",", &saveptr);
    
    while (token != NULL && field < 15) {
        switch (field) {
            case 1: // Time
                if (strlen(token) >= 6) {
                    g_gps_data.hour = (token[0] - '0') * 10 + (token[1] - '0');
                    g_gps_data.minute = (token[2] - '0') * 10 + (token[3] - '0');
                    g_gps_data.second = (token[4] - '0') * 10 + (token[5] - '0');
                }
                break;
            case 2: // Latitude
                if (strlen(token) > 0) {
                    g_gps_data.latitude = atof(token);
                }
                break;
            case 3: // N/S
                if (token[0] == 'S') {
                    g_gps_data.latitude = -g_gps_data.latitude;
                }
                break;
            case 4: // Longitude
                if (strlen(token) > 0) {
                    g_gps_data.longitude = atof(token);
                }
                break;
            case 5: // E/W
                if (token[0] == 'W') {
                    g_gps_data.longitude = -g_gps_data.longitude;
                }
                break;
            case 6: // Fix quality
                g_gps_data.fix_valid = (token[0] != '0');
                break;
            case 7: // Number of satellites
                g_gps_data.satellites = atoi(token);
                break;
            case 9: // Altitude
                if (strlen(token) > 0) {
                    g_gps_data.altitude = atof(token);
                }
                break;
        }
        token = strtok_r(NULL, ",", &saveptr);
        field++;
    }
}

/**
 * @brief Parse GPRMC sentence (Recommended Minimum)
 */
static void parse_gprmc(const char *sentence)
{
    char *token;
    char *saveptr;
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    int field = 0;
    token = strtok_r(buffer, ",", &saveptr);
    
    while (token != NULL && field < 12) {
        switch (field) {
            case 1: // Time
                if (strlen(token) >= 6) {
                    g_gps_data.hour = (token[0] - '0') * 10 + (token[1] - '0');
                    g_gps_data.minute = (token[2] - '0') * 10 + (token[3] - '0');
                    g_gps_data.second = (token[4] - '0') * 10 + (token[5] - '0');
                }
                break;
            case 2: // Status
                g_gps_data.fix_valid = (token[0] == 'A');
                break;
            case 9: // Date
                if (strlen(token) >= 6) {
                    g_gps_data.day = (token[0] - '0') * 10 + (token[1] - '0');
                    g_gps_data.month = (token[2] - '0') * 10 + (token[3] - '0');
                    g_gps_data.year = 2000 + (token[4] - '0') * 10 + (token[5] - '0');
                }
                break;
        }
        token = strtok_r(NULL, ",", &saveptr);
        field++;
    }
}

/**
 * @brief Parse NMEA sentence
 */
static void parse_nmea_sentence(const char *sentence)
{
    if (!nmea_verify_checksum(sentence)) {
        ESP_LOGW(TAG, "Invalid checksum: %s", sentence);
        return;
    }
    
    if (strncmp(sentence, "$GPGGA", 6) == 0 || strncmp(sentence, "$GNGGA", 6) == 0) {
        parse_gpgga(sentence);
        g_data_valid = true;
    } else if (strncmp(sentence, "$GPRMC", 6) == 0 || strncmp(sentence, "$GNRMC", 6) == 0) {
        parse_gprmc(sentence);
        g_data_valid = true;
    }
}

/**
 * @brief GPS UART receive task
 */
static void gps_uart_task(void *arg)
{
    uint8_t data[128];
    
    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, data, sizeof(data) - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = data[i];
                
                if (c == '\n' || c == '\r') {
                    if (nmea_buffer_pos > 0) {
                        nmea_buffer[nmea_buffer_pos] = '\0';
                        parse_nmea_sentence(nmea_buffer);
                        nmea_buffer_pos = 0;
                    }
                } else if (nmea_buffer_pos < sizeof(nmea_buffer) - 1) {
                    nmea_buffer[nmea_buffer_pos++] = c;
                }
            }
        }
    }
}

void gps_init(void)
{
    ESP_LOGI(TAG, "Initializing GPS driver...");
    
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = GPS_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    // Configure PPS pin
    gpio_config_t pps_config = {
        .pin_bit_mask = (1ULL << GPS_PPS_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&pps_config);
    
    // Install ISR service and add handler for PPS pin
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPS_PPS_PIN, pps_isr_handler, NULL);
    
    // Create UART receive task
    xTaskCreate(gps_uart_task, "gps_uart_task", 4096, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "GPS driver initialized");
}

bool gps_get_data(gps_data_t *data)
{
    if (!g_data_valid || !data) {
        return false;
    }
    
    memcpy(data, &g_gps_data, sizeof(gps_data_t));
    return true;
}

uint64_t gps_get_pps_timestamp(void)
{
    return g_pps_timestamp;
}

bool gps_has_fix(void)
{
    return g_data_valid && g_gps_data.fix_valid;
}
