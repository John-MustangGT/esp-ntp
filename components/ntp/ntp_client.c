#include "ntp_client.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <sys/time.h>

static const char *TAG = "NTP_CLIENT";

#define NTP_TIMESTAMP_DELTA 2208988800ull  // Seconds between 1900 and 1970

static char ntp_server[128] = NTP_DEFAULT_SERVER;
static bool last_sync_success = false;
static time_t last_sync_time = 0;

esp_err_t ntp_client_init(const char *server) {
    if (server != NULL && strlen(server) > 0) {
        strncpy(ntp_server, server, sizeof(ntp_server) - 1);
        ntp_server[sizeof(ntp_server) - 1] = '\0';
    }
    ESP_LOGI(TAG, "NTP client initialized with server: %s", ntp_server);
    return ESP_OK;
}

static uint32_t htonl_custom(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8) |
           ((hostlong & 0x00FF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

esp_err_t ntp_sync(struct timeval *time_out) {
    if (time_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Resolve NTP server
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int err = getaddrinfo(ntp_server, "123", &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed for %s: %d", ntp_server, err);
        return ESP_FAIL;
    }

    // Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        freeaddrinfo(res);
        return ESP_FAIL;
    }

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = NTP_TIMEOUT_MS / 1000;
    timeout.tv_usec = (NTP_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Prepare NTP request packet
    ntp_packet_t packet;
    memset(&packet, 0, sizeof(packet));
    packet.li_vn_mode = 0x1B;  // LI=0, VN=3, Mode=3 (client)

    // Send NTP request
    if (sendto(sockfd, &packet, sizeof(packet), 0, res->ai_addr, res->ai_addrlen) < 0) {
        ESP_LOGE(TAG, "Failed to send NTP request");
        close(sockfd);
        freeaddrinfo(res);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "NTP request sent to %s", ntp_server);

    // Receive NTP response
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int len = recvfrom(sockfd, &packet, sizeof(packet), 0, res->ai_addr, &addrlen);
    close(sockfd);
    freeaddrinfo(res);

    if (len < sizeof(packet)) {
        ESP_LOGE(TAG, "Failed to receive NTP response (len=%d)", len);
        return ESP_FAIL;
    }

    // Extract timestamp from response
    uint32_t trans_sec = htonl_custom(packet.trans_timestamp_sec);
    uint32_t trans_frac = htonl_custom(packet.trans_timestamp_frac);

    // Convert to Unix timestamp
    time_t unix_time = trans_sec - NTP_TIMESTAMP_DELTA;
    uint32_t usec = ((uint64_t)trans_frac * 1000000) >> 32;

    time_out->tv_sec = unix_time;
    time_out->tv_usec = usec;

    ESP_LOGI(TAG, "NTP sync successful: %ld.%06ld", unix_time, usec);
    last_sync_success = true;
    last_sync_time = unix_time;

    return ESP_OK;
}

esp_err_t ntp_sync_system_time(void) {
    struct timeval tv;
    esp_err_t err = ntp_sync(&tv);
    
    if (err == ESP_OK) {
        settimeofday(&tv, NULL);
        ESP_LOGI(TAG, "System time updated from NTP");
        
        // Log the current time
        time_t now;
        struct tm timeinfo;
        char strftime_buf[64];
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", strftime_buf);
    } else {
        last_sync_success = false;
        ESP_LOGE(TAG, "Failed to sync system time from NTP");
    }

    return err;
}

bool ntp_is_synced(void) {
    return last_sync_success;
}

uint32_t ntp_get_seconds_since_sync(void) {
    if (!last_sync_success) {
        return UINT32_MAX;
    }
    time_t now;
    time(&now);
    return (uint32_t)(now - last_sync_time);
}
