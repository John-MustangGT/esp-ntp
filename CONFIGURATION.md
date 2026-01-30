# Configuration Guide

This document explains various configuration options and how to customize the ESP32-S3 GPS NTP Server for your specific needs.

## WiFi Configuration

### Method 1: Edit sdkconfig.defaults (Recommended)

Edit `sdkconfig.defaults` before building:

```ini
CONFIG_WIFI_SSID="MyNetwork"
CONFIG_WIFI_PASSWORD="MySecurePassword"
```

### Method 2: Using menuconfig

If using ESP-IDF:

```bash
idf.py menuconfig
```

Navigate to: `WiFi Configuration` → Set SSID and Password

### Method 3: Programmatic Configuration

For advanced users, edit `src/main.cpp`:

```cpp
// Multiple WiFi networks with failover
const char* wifi_ssids[] = {"Network1", "Network2", "Network3"};
const char* wifi_passwords[] = {"Pass1", "Pass2", "Pass3"};
```

## GPS Configuration

### Change UART Pins

If you need to use different GPIO pins, edit `src/gps_driver.c`:

```c
#define GPS_TX_PIN          43  // Change to your TX pin
#define GPS_RX_PIN          44  // Change to your RX pin
#define GPS_PPS_PIN         1   // Change to your PPS pin
```

### Change Baud Rate

Default is 9600. To change, edit `src/gps_driver.c`:

```c
#define GPS_UART_BAUD_RATE  115200  // Or 38400, 57600, etc.
```

**Note**: You may need to configure your GPS module to match this baud rate.

### NMEA Sentence Selection

The driver currently parses GPGGA and GPRMC. To add more sentences, edit `parse_nmea_sentence()` in `src/gps_driver.c`:

```c
if (strncmp(sentence, "$GPGSV", 6) == 0) {
    // Parse satellite info
}
```

## OLED Display Configuration

### Change I2C Pins

Edit `src/oled_display.c`:

```c
#define I2C_MASTER_SDA_IO   5   // Change to your SDA pin
#define I2C_MASTER_SCL_IO   6   // Change to your SCL pin
```

### Change I2C Address

If your OLED uses a different address, edit `src/oled_display.c`:

```c
#define OLED_I2C_ADDRESS    0x3D  // Common alternative: 0x3D
```

### Change Display Size

For 128x64 displays, edit `src/oled_display.c`:

```c
#define OLED_HEIGHT         64    // Instead of 32
#define OLED_PAGES          (OLED_HEIGHT / 8)  // Will be 8
```

Then update initialization:

```c
oled_write_command(0x3F);  // Multiplex ratio for 64 rows
oled_write_command(0x12);  // COM pins configuration for 64 rows
```

### Customize Display Content

Edit the main loop in `src/main.cpp` to change what's displayed:

```cpp
oled_display_text(0, "My NTP Server", 0);
oled_display_text(1, time_str, 0);
oled_display_text(2, gps_str, 0);
oled_display_text(3, ip_address_str, 0);  // Add IP display
```

## NTP Server Configuration

### Change NTP Port

By default, NTP uses port 123. To change, edit `src/ntp_server.c`:

```c
#define NTP_PORT 1123  // Custom port
```

**Note**: Clients will need to specify this port when querying.

### Adjust Stratum Level

For testing or specific configurations, edit `src/ntp_server.c`:

```c
response.stratum = 1;  // Change to 0, 1, or 2
```

**Stratum Levels**:
- 0: Reference clock (atomic/GPS)
- 1: Primary server (this project's default)
- 2: Secondary server

### Configure Precision

Edit `src/ntp_server.c`:

```c
response.precision = -20;  // ~1 microsecond
// -10 = ~1 millisecond
// -20 = ~1 microsecond
// -30 = ~1 nanosecond (theoretical)
```

### Reference ID

Change the 4-character reference ID in `src/ntp_server.c`:

```c
response.ref_id = htonl(0x47505300);  // "GPS"
// Alternatives:
// 0x50505300 = "PPS"
// 0x474f4553 = "GOES" (satellite)
// 0x47414c00 = "GAL" (Galileo)
```

## Network Configuration

### Static IP Address

To use a static IP instead of DHCP, edit `src/main.cpp`:

```cpp
// After esp_netif_create_default_wifi_sta():
esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

esp_netif_dhcpc_stop(netif);

esp_netif_ip_info_t ip_info;
IP4_ADDR(&ip_info.ip, 192, 168, 1, 100);        // Static IP
IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);          // Gateway
IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);   // Netmask

esp_netif_set_ip_info(netif, &ip_info);

// Set DNS servers
esp_netif_dns_info_t dns_info;
IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 8, 8);  // Google DNS
esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
```

### Hostname Configuration

Set a custom hostname in `src/main.cpp`:

```cpp
esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
esp_netif_set_hostname(netif, "gps-ntp-server");
```

## Performance Tuning

### Task Priorities

Adjust FreeRTOS task priorities in respective source files:

```cpp
// GPS task - higher priority for time-critical data
xTaskCreate(gps_uart_task, "gps_uart_task", 4096, NULL, 15, NULL);

// NTP task - medium priority
xTaskCreate(ntp_server_task, "ntp_server_task", 4096, NULL, 10, NULL);

// Display task - lower priority
xTaskCreate(display_task, "display_task", 4096, NULL, 5, NULL);
```

### Stack Sizes

If you encounter stack overflow, increase task stack sizes:

```cpp
xTaskCreate(task_function, "task_name", 8192, NULL, 10, NULL);
//                                       ^^^^ Increase this value
```

### CPU Frequency

For better performance or lower power, edit `sdkconfig.defaults`:

```ini
# Maximum performance
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240=y

# Balanced
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_160=y

# Low power
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_80=y
```

## Logging Configuration

### Change Log Levels

Edit `sdkconfig.defaults`:

```ini
# Debug level (very verbose)
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
CONFIG_LOG_DEFAULT_LEVEL=4

# Info level (default)
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_LOG_DEFAULT_LEVEL=3

# Warning level (quieter)
CONFIG_LOG_DEFAULT_LEVEL_WARN=y
CONFIG_LOG_DEFAULT_LEVEL=2

# Error level (quietest)
CONFIG_LOG_DEFAULT_LEVEL_ERROR=y
CONFIG_LOG_DEFAULT_LEVEL=1
```

### Per-Component Log Levels

In code, set log levels per tag:

```c
esp_log_level_set("GPS", ESP_LOG_DEBUG);
esp_log_level_set("NTP", ESP_LOG_INFO);
esp_log_level_set("OLED", ESP_LOG_WARN);
```

## Build Configuration

### Partition Table

To change storage allocation, edit `partitions.csv`:

```csv
# Name,   Type, SubType, Offset,  Size,     Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x200000,  # 2MB for app
storage,  data, spiffs,  0x210000,0x1F0000, # ~2MB for files
```

### Compiler Optimizations

Edit `platformio.ini` or `sdkconfig.defaults`:

```ini
# Size optimization (smaller binary)
CONFIG_COMPILER_OPTIMIZATION_SIZE=y

# Performance optimization (faster execution)
CONFIG_COMPILER_OPTIMIZATION_PERF=y

# Debug optimization (easier debugging)
CONFIG_COMPILER_OPTIMIZATION_DEBUG=y
```

## Advanced Features

### Add Authentication

For secure NTP (NTS), you would need to implement:

1. TLS/SSL support
2. Key management
3. Cookie generation
4. Packet encryption

This is beyond the scope of this basic implementation.

### Add Statistics Logging

To log NTP statistics to SD card or flash:

```cpp
// In src/main.cpp
void log_stats_task(void *arg) {
    while(1) {
        ntp_stats_t stats;
        ntp_get_stats(&stats);
        
        // Log to file or send to server
        ESP_LOGI(TAG, "Stats: Req=%lu, Resp=%lu, Err=%lu",
                 stats.request_count,
                 stats.response_count,
                 stats.error_count);
        
        vTaskDelay(pdMS_TO_TICKS(60000));  // Every minute
    }
}
```

### Multi-GPS Support

For redundancy, you can add a second GPS:

```c
// Add second UART
#define GPS2_UART_NUM       UART_NUM_2
#define GPS2_TX_PIN         17
#define GPS2_RX_PIN         18
#define GPS2_PPS_PIN        8
```

Then implement voting/averaging logic in NTP server.

## Backup Configuration

Save your custom configuration:

```bash
# ESP-IDF
idf.py save-defconfig

# Backup important files
cp sdkconfig.defaults my_config_backup.txt
cp src/main.cpp my_main_backup.cpp
```

## Restore Default Configuration

To restore defaults:

```bash
git checkout sdkconfig.defaults
git checkout src/main.cpp
git checkout src/gps_driver.c
git checkout src/ntp_server.c
git checkout src/oled_display.c
```

## Common Configuration Scenarios

### Scenario 1: Indoor Testing (No GPS)

For testing without GPS signal:

```cpp
// In src/ntp_server.c, use system time instead of GPS
struct timeval tv;
gettimeofday(&tv, NULL);
// Use tv for NTP responses
```

### Scenario 2: Multiple WiFi Networks

```cpp
// Try multiple networks in sequence
const char* networks[][2] = {
    {"Home_WiFi", "home_pass"},
    {"Office_WiFi", "office_pass"},
    {"Backup_WiFi", "backup_pass"}
};

for(int i = 0; i < 3; i++) {
    // Try to connect to networks[i]
}
```

### Scenario 3: Minimal Power Consumption

```cpp
// Reduce display updates
vTaskDelay(pdMS_TO_TICKS(5000));  // Update every 5 seconds

// Lower CPU frequency
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_80=y

// Disable WiFi sleep
esp_wifi_set_ps(WIFI_PS_NONE);  // Or WIFI_PS_MIN_MODEM
```

## Validation

After making configuration changes:

1. **Build**: Ensure project compiles without errors
2. **Flash**: Upload to device
3. **Monitor**: Check serial output for issues
4. **Test**: Verify all features work as expected
5. **Measure**: Check performance metrics (accuracy, response time)

## Support

For configuration help:
- See [README.md](README.md) for general information
- Check [QUICKSTART.md](QUICKSTART.md) for getting started
- Visit GitHub Issues for specific problems
