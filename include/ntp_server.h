/**
 * @file ntp_server.h
 * @brief NTP server implementation for Stratum 1 GPS time source
 */

#ifndef NTP_SERVER_H
#define NTP_SERVER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t request_count;
    uint32_t response_count;
    uint32_t error_count;
} ntp_stats_t;

/**
 * @brief Initialize NTP server
 */
void ntp_server_init(void);

/**
 * @brief Get NTP server statistics
 * @param stats Pointer to ntp_stats_t structure to fill
 */
void ntp_get_stats(ntp_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif // NTP_SERVER_H
