# ESP-NTP

A dual-core NTP/GPS time synchronization system for the Seeed Studios XIAO ESP32-S3 with GPS discipline and PPS (Pulse Per Second) precision.

## Features

- **Dual-Core Architecture**: Core 0 handles NTP and web services; Core 1 manages GPS and PPS
- **Multiple Time Sources**: NTP, GPS, and GPS+PPS with automatic fallback
- **Web Configuration Portal**: Initial setup via WiFi AP mode
- **Persistent Configuration**: NVS storage with checksum validation
- **OLED Display**: 128x32 I2C display for status and time
- **OTA Updates**: 3MB partition for over-the-air firmware updates

## Hardware Requirements

- **Microcontroller**: Seeed Studios XIAO ESP32-S3
- **GPS Module**: ATGM336H (UART, 9600 baud)
- **Display**: I2C OLED 128x32 (address 0x3C)
- **PPS Signal**: Connected to GPIO1 from GPS module

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- USB-C cable for programming

### Building

```bash
pio run
```

### Uploading

```bash
pio run --target upload
```

### Monitoring

```bash
pio device monitor
```

## First Boot Configuration

1. On first boot (or invalid NVS), device starts in AP mode
2. Connect to WiFi network: `ESP-NTP-Config` (password: `configure`)
3. Navigate to: `http://192.168.4.1`
4. Enter your WiFi credentials, NTP server, and timezone
5. Click "Save & Reboot"
6. Device reboots and connects to your WiFi network

## Architecture

### Core 0 (Network & UI)
- NTP client synchronization
- HTTP web server (configuration API)
- I2C OLED display management
- System time updates via `settimeofday()`
- WiFi provisioning (AP/Station modes)

### Core 1 (GPS & Precision)
- UART communication with ATGM336H GPS
- NMEA sentence parsing (GGA, RMC)
- PPS interrupt handler on GPIO1
- Nanosecond-precision timestamp capture
- GPS time validation and signaling

### Inter-Core Communication
- FreeRTOS queues for thread-safe data exchange
- GPS validity status and time data from Core 1 → Core 0
- Configuration updates from Core 0 → Core 1

## Components

- **nvs_storage**: Configuration storage with CRC32 checksum validation
- **wifi_provisioning**: WiFi AP and station mode management
- **web**: HTTP server with configuration portal and status API
- **ntp**: NTP client with UDP socket implementation
- **gps**: NMEA parser for ATGM336H GPS module
- **pps**: PPS interrupt handler for precision timing
- **oled**: I2C OLED display driver (128x32)
- **time_sync**: Time source orchestration (GPS+PPS > GPS > NTP)

## Flash Partitions

| Partition | Size | Purpose |
|-----------|------|---------|
| nvs | 2MB | Configuration and statistics storage |
| ota_0 | 3MB | Primary application firmware |
| ota_1 | 3MB | OTA update partition |

## API Endpoints

- `GET /` - Configuration portal (AP mode) or status page
- `POST /save` - Save configuration and reboot (AP mode)
- `GET /api/status` - JSON status information

## Time Synchronization Priority

1. **GPS+PPS**: GPS time with PPS discipline (highest accuracy)
2. **GPS**: GPS time without PPS (high accuracy)
3. **NTP**: Network time protocol (fallback)

## Development

See [`.github/copilot-instructions.md`](.github/copilot-instructions.md) for detailed development guidelines and architecture information.

## License

MIT License - see [LICENSE](LICENSE) for details

## Author

John Orthoefer
