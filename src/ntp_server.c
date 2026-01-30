/**
 * @file ntp_server.c
 * @brief NTP server implementation for Stratum 1 GPS time source
 */

#include "ntp_server.h"
#include "gps_driver.h"
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "esp_timer.h"

static const char *TAG = "NTP";

#define NTP_PORT 123
#define NTP_TIMESTAMP_DELTA 2208988800ull

// NTP packet structure (48 bytes)
typedef struct {
    uint8_t li_vn_mode;      // Leap Indicator, Version, Mode
    uint8_t stratum;         // Stratum level
    uint8_t poll;            // Poll interval
    uint8_t precision;       // Precision
    uint32_t root_delay;     // Root delay
    uint32_t root_dispersion; // Root dispersion
    uint32_t ref_id;         // Reference ID
    uint32_t ref_timestamp_sec; // Reference timestamp (seconds)
    uint32_t ref_timestamp_frac; // Reference timestamp (fraction)
    uint32_t orig_timestamp_sec; // Originate timestamp (seconds)
    uint32_t orig_timestamp_frac; // Originate timestamp (fraction)
    uint32_t rx_timestamp_sec;   // Receive timestamp (seconds)
    uint32_t rx_timestamp_frac;  // Receive timestamp (fraction)
    uint32_t tx_timestamp_sec;   // Transmit timestamp (seconds)
    uint32_t tx_timestamp_frac;  // Transmit timestamp (fraction)
} __attribute__((packed)) ntp_packet_t;

// Statistics
static ntp_stats_t g_stats = {0};

/**
 * @brief Convert GPS time to NTP timestamp
 */
static void gps_to_ntp_timestamp(gps_data_t *gps, uint32_t *sec, uint32_t *frac)
{
    struct tm timeinfo = {0};
    timeinfo.tm_year = gps->year - 1900;
    timeinfo.tm_mon = gps->month - 1;
    timeinfo.tm_mday = gps->day;
    timeinfo.tm_hour = gps->hour;
    timeinfo.tm_min = gps->minute;
    timeinfo.tm_sec = gps->second;
    
    time_t timestamp = mktime(&timeinfo);
    
    // Convert to NTP timestamp (seconds since 1900-01-01)
    *sec = htonl((uint32_t)(timestamp + NTP_TIMESTAMP_DELTA));
    
    // Calculate fraction based on microseconds from PPS
    uint64_t pps_time = gps_get_pps_timestamp();
    uint64_t current_time = esp_timer_get_time();
    uint64_t usec_since_pps = current_time - pps_time;
    
    // Convert microseconds to NTP fraction (2^32 units per second)
    uint64_t frac_val = (usec_since_pps * 4294967296ULL) / 1000000ULL;
    *frac = htonl((uint32_t)frac_val);
}

/**
 * @brief NTP server task
 */
static void ntp_server_task(void *pvParameters)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int sock;
    ntp_packet_t packet;
    
    ESP_LOGI(TAG, "Creating NTP server socket...");
    
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    
    // Set socket options
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    // Bind to NTP port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(NTP_PORT);
    
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "NTP server listening on port %d", NTP_PORT);
    
    while (1) {
        // Receive NTP request
        int len = recvfrom(sock, &packet, sizeof(packet), 0,
                          (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (len == sizeof(ntp_packet_t)) {
            g_stats.request_count++;
            
            // Get current GPS data
            gps_data_t gps_data;
            if (gps_get_data(&gps_data) && gps_data.fix_valid) {
                // Prepare NTP response
                ntp_packet_t response;
                memset(&response, 0, sizeof(response));
                
                // Set leap indicator, version, and mode
                response.li_vn_mode = (0 << 6) | (4 << 3) | 4; // No leap warning, NTPv4, server mode
                response.stratum = 1; // Stratum 1 (GPS reference)
                response.poll = 6;    // 2^6 = 64 seconds
                response.precision = -20; // ~1 microsecond precision
                
                // Reference ID for GPS
                response.ref_id = htonl(0x47505300); // "GPS"
                
                // Get NTP timestamps
                uint32_t ref_sec, ref_frac;
                gps_to_ntp_timestamp(&gps_data, &ref_sec, &ref_frac);
                
                response.ref_timestamp_sec = ref_sec;
                response.ref_timestamp_frac = ref_frac;
                
                // Copy originate timestamp from request
                response.orig_timestamp_sec = packet.tx_timestamp_sec;
                response.orig_timestamp_frac = packet.tx_timestamp_frac;
                
                // Set receive and transmit timestamps
                response.rx_timestamp_sec = ref_sec;
                response.rx_timestamp_frac = ref_frac;
                response.tx_timestamp_sec = ref_sec;
                response.tx_timestamp_frac = ref_frac;
                
                // Send response
                int sent = sendto(sock, &response, sizeof(response), 0,
                                (struct sockaddr *)&client_addr, client_addr_len);
                
                if (sent == sizeof(response)) {
                    g_stats.response_count++;
                    ESP_LOGD(TAG, "Sent NTP response to client");
                } else {
                    g_stats.error_count++;
                    ESP_LOGW(TAG, "Failed to send NTP response");
                }
            } else {
                g_stats.error_count++;
                ESP_LOGW(TAG, "No GPS fix available, cannot respond to NTP request");
            }
        }
    }
}

void ntp_server_init(void)
{
    ESP_LOGI(TAG, "Initializing NTP server...");
    xTaskCreate(ntp_server_task, "ntp_server_task", 4096, NULL, 5, NULL);
}

void ntp_get_stats(ntp_stats_t *stats)
{
    if (stats) {
        memcpy(stats, &g_stats, sizeof(ntp_stats_t));
    }
}
