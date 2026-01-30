# ESP-NTP: Copilot Instructions

## Project Overview
**esp-ntp** is a dual-core NTP/GPS time synchronization system running on the Seeed Studios XIAO ESP32-S3. It combines NTP (Network Time Protocol) for internet-based time sync with GPS discipline and PPS (Pulse Per Second) signals for precision timekeeping. Core 0 handles NTP and web services; Core 1 manages GPS and PPS interrupts.

---

## Hardware & Peripherals

### Target Platform
- **Microcontroller**: Seeed Studios XIAO ESP32-S3 (ARM Cortex-M7 dual-core @ 240 MHz)
- **GPS Module**: ATGM336H (serial interface, 9600 baud default)
- **Display**: I2C OLED 128x32 (address 0x3C typical)
- **PPS Interrupt**: GPIO1 (active high, 1Hz square wave from GPS)

### I2C Communication
- OLED uses standard I2C protocol (address 0x3C)
- Keep I2C transactions non-blocking to avoid starving other tasks
- Display refresh should be rate-limited (≤30Hz typical)

### GPIO Assignments
- **GPIO1**: PPS interrupt input from ATGM336H (rising edge detection)
- Configure interrupt handler in Core 1 for immediate timestamp capture

---

## Dual-Core Architecture

### Core 0: Network & UI Tasks
- NTP client implementation with configurable servers and sync intervals
- Web server serving static files and configuration API
- I2C OLED display management (status, time display, diagnostics)
- Time synchronization via `settimeofday()` after GPS or NTP validation
- Non-blocking task design: use FreeRTOS queues for cross-core communication
- WiFi provisioning: AP mode on invalid NVS, station mode for normal operation

### Core 1: GPS & Precision Tasks
- UART communication with ATGM336H (serial RX on default pins)
- PPS interrupt handler on GPIO1 for nanosecond-precision timestamps
- Parse GPS NMEA sentences (GGA, RMC for position and time)
- Signal Core 0 via queue when GPS time becomes valid
- Execute immediately on PPS edges to capture system time references

### Inter-Core Communication
- Use FreeRTOS queues for thread-safe data exchange
- Core 1 → Core 0: GPS validity status, latitude/longitude, UTC time from NMEA
- Core 0 → Core 1: Configuration updates (poll interval, etc.)
- Avoid mutex contention; prefer message passing

---

## Architecture & Key Components

### Anticipated Structure
- **components/** - ESP-IDF components for each subsystem
  - **ntp/** - NTP client, server polling, time validation
  - **gps/** - NMEA parser, UART configuration for ATGM336H
  - **pps/** - PPS interrupt handler and timestamp capture
  - **oled/** - I2C display driver and UI rendering (128x32)
  - **web/** - HTTP server, static file serving, config API endpoints
  - **time_sync/** - Time synchronization orchestration across Core 0/1
  - **nvs_storage/** - NVS flash access for configuration/statistics (2MB partition)
- **main/** - Application entry point and core coordination
- **partitions.csv** - Flash partition table (3MB app, 3MB OTA, 2MB NVS)
- **examples/** - Demonstration sketches
- **test/** - Unit tests with mock GPS/NTP data

### Design Principles
- Lightweight implementation suitable for memory-constrained embedded systems
- Modular design: separate time sync logic from network communication
- Minimal dependencies to reduce firmware footprint

---

## Development Workflows

### Boot Sequence & NVS Configuration
- **NVS Validation**: On startup, validate stored configuration with checksum verification
- **AP Mode Fallback**: If NVS invalid or checksum fails, boot into Access Point (AP) mode
- **Configuration Portal**: Web server listens on `192.168.4.1` (default AP gateway) for WiFi setup
- **User Configuration**: Web UI allows entry of SSID, WiFi password, NTP server, timezone, etc.
- **Checksum Storage**: Write checksum alongside config data; validate before use to prevent corrupted boots
- **Reboot & Connect**: After configuration, save to NVS with new checksum and reboot to station mode
- **Station Mode**: Normal operation connects to WiFi with stored credentials, starts NTP/GPS sync

### Build & Test
- **Framework**: PlatformIO with Espressif ESP-IDF (not Arduino)
- **Build Command**: `pio run` (PlatformIO for XIAO ESP32-S3 board)
- **Upload to Device**: `pio run --target upload`
- **Serial Debugging**: Monitor at 115200 baud via `pio device monitor`
- **Configuration**: `platformio.ini` specifies board, framework (esp-idf), and dependencies
- **Partition Scheme**: Custom partitions defined in `sdkconfig` or partition table file
  - 3MB application firmware
  - 3MB OTA (over-the-air update) partition
  - 2MB NVS (configuration/statistics storage)

### Code Organization
- C/C++ code using Espressif ESP-IDF (not Arduino)
- ESP-IDF component structure: each subsystem is an ESP-IDF component under `components/`
- Component `CMakeLists.txt` defines dependencies, includes, and sources
- Main application in `main/` component
- Use `#include` guards or `#pragma once` for header files
- Platform configuration via `sdkconfig` (kconfig-based system)
- Partition table defined in `partitions.csv` or via `idf.py` menuconfig

---

## Project-Specific Conventions

### Naming Patterns
- Functions: `snake_case` (e.g., `ntp_sync_time()`, `set_time_zone()`)
- Constants/Macros: `UPPER_SNAKE_CASE` (e.g., `NTP_PORT`, `SYNC_INTERVAL_MS`)
- Classes (if used): `PascalCase` (e.g., `NTPClient`)

### Key Patterns to Follow
- **Non-blocking operations**: Embedded systems typically avoid blocking calls; design async where possible
- **Memory efficiency**: Minimize dynamic allocation; prefer stack allocation or fixed buffers
- **Configuration**: Use defines/macros for compile-time configuration

### External Dependencies
- Espressif ESP-IDF core library and FreeRTOS kernel
- NTP implementation: UDP sockets via lwIP stack (built-in)
- GPS NMEA parsing: custom lightweight parser or TinyGPS++ library
- I2C display: U8g2 or Adafruit_SSD1306 libraries (ported to ESP-IDF)
- WiFi/networking: Built-in ESP32 WiFi stack
- NVS (Non-Volatile Storage): Built-in ESP-IDF NVS library for config/statistics

---

## Integration Points

### Critical Interfaces
- **NTP Server Communication**: UDP socket implementation to NTP server (typically port 123)
- **System Time Updates**: Integration with ESP's system clock via `settimeofday()` or equivalent
- **WiFi Dependencies**: Requires active network connection before time sync
- **GPS UART**: 9600 baud serial communication with ATGM336H on default RX pin
- **PPS/GPIO1**: Rising edge interrupt triggers timestamp capture in Core 1
- **I2C OLED**: Non-blocking display updates from Core 0

### Testing Strategy
- Unit tests for time calculation logic
- Integration tests with mock NTP servers or hardware testing with actual time servers
- Validate time precision and sync reliability across reboots

---

## Immediate Tasks for New Contributors
1. Establish project directory structure (src, include, examples, tests)
2. Implement core NTP client to parse NTP packets
3. Create time synchronization management module
4. Write example sketch demonstrating NTP sync
5. Add README with build instructions and usage examples

---

## Questions Before Starting Work?
- Target ESP variant (ESP8266, ESP32)?
- Is this for Arduino IDE, PlatformIO, or custom SDK?
- Any specific NTP server configuration requirements?

---

## Notes
- PPS timing precision is critical; keep interrupt handler in Core 1 lightweight and fast
- Dual-core synchronization requires careful queue management to avoid race conditions
- GPS cold start may take 30-60 seconds; implement graceful NTP fallback
