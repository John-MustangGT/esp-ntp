# ESP32-S3 GPS NTP Server

A Stratum 1 NTP (Network Time Protocol) server based on the ESP32-S3 microcontroller and GPS module. This project provides high-precision network time synchronization using GPS as the reference time source.

## Features

- **Stratum 1 NTP Server**: Direct GPS time reference for maximum accuracy
- **PPS (Pulse Per Second)**: High-precision timing synchronization
- **OLED Display**: Real-time status display showing GPS fix, time, and NTP statistics
- **WiFi Connectivity**: Serves time to network clients over WiFi
- **ESP-IDF Framework**: Built on Espressif's official IoT Development Framework

## Hardware Requirements

### Components

1. **Seeed Studios Xiao ESP32-S3**
   - Compact ESP32-S3 development board
   - Dual-core processor
   - Built-in WiFi

2. **Teyleten ATGM336M GPS Module**
   - UART Interface: GPIO 43 (TX), GPIO 44 (RX)
   - PPS Output: GPIO 1
   - Supports NMEA protocol

3. **0.91" 128x32 OLED Display (SSD1306)**
   - I2C Interface: GPIO 5 (SDA), GPIO 6 (SCL)
   - 4-line text display for status information

### Pin Configuration

| Component | Function | GPIO Pin |
|-----------|----------|----------|
| GPS Module | UART TX | GPIO 43 |
| GPS Module | UART RX | GPIO 44 |
| GPS Module | PPS | GPIO 1 |
| OLED Display | I2C SDA | GPIO 5 |
| OLED Display | I2C SCL | GPIO 6 |

## Software Requirements

- [PlatformIO](https://platformio.org/) - Build system and IDE
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/) - Espressif IoT Development Framework
- Python 3.7 or later (for ESP-IDF tools)

## Building and Flashing

### Using PlatformIO

1. **Clone the repository**:
   ```bash
   git clone https://github.com/John-MustangGT/esp-ntp.git
   cd esp-ntp
   ```

2. **Configure WiFi credentials**:
   Edit `sdkconfig.defaults` and update:
   ```
   CONFIG_ESP_WIFI_SSID="YourWiFiSSID"
   CONFIG_ESP_WIFI_PASSWORD="YourWiFiPassword"
   ```

3. **Build the project**:
   ```bash
   pio run
   ```

4. **Upload to device**:
   ```bash
   pio run --target upload
   ```

5. **Monitor serial output**:
   ```bash
   pio device monitor
   ```

### Using ESP-IDF directly

1. **Set up ESP-IDF environment**:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

2. **Configure the project**:
   ```bash
   idf.py menuconfig
   ```
   Navigate to "WiFi Configuration" and set your SSID and password.

3. **Build and flash**:
   ```bash
   idf.py build
   idf.py flash monitor
   ```

## Usage

### Initial Setup

1. Connect the hardware according to the pin configuration table
2. Power on the device
3. Wait for GPS to acquire a fix (may take a few minutes outdoors)
4. The OLED display will show:
   - Line 1: "GPS NTP Server"
   - Line 2: Current UTC time
   - Line 3: GPS fix status and satellite count
   - Line 4: NTP request count

### Configuring NTP Clients

Once the server is running and has a GPS fix, configure your devices to use it as an NTP server:

**Linux/macOS**:
```bash
sudo ntpdate <ESP32_IP_ADDRESS>
```

**Windows**:
1. Open Date & Time settings
2. Add the ESP32 IP address as a time server
3. Sync now

**Router/Network Configuration**:
Configure your router's DHCP settings to provide the ESP32's IP address as the NTP server to all network clients.

## System Architecture

### GPS Driver (`gps_driver.c`)
- Receives and parses NMEA sentences from GPS module via UART
- Handles PPS interrupt for precise timing
- Provides GPS time and fix status to other components

### NTP Server (`ntp_server.c`)
- Implements NTP protocol (RFC 5905)
- Operates as Stratum 1 server with GPS reference
- Uses PPS timestamps for sub-millisecond accuracy
- Handles NTP client requests and responds with accurate time

### OLED Display (`oled_display.c`)
- SSD1306 driver for 128x32 OLED
- Displays system status in real-time
- Shows GPS fix, time, and NTP statistics

### Main Application (`main.cpp`)
- Initializes all system components
- Manages WiFi connectivity
- Coordinates GPS, NTP, and display updates

## Performance

- **Stratum Level**: 1 (GPS reference)
- **Precision**: ~1 microsecond (with PPS)
- **Network Latency**: Typically 1-5ms on local network
- **GPS Acquisition**: 30-120 seconds (cold start)
- **GPS Re-acquisition**: 1-30 seconds (warm start)

## Troubleshooting

### GPS Not Getting Fix
- Ensure GPS antenna has clear view of sky
- Check GPS module connections (TX, RX, PPS, Power, Ground)
- Wait longer (up to 15 minutes for first fix)
- Check serial monitor for GPS NMEA sentences

### WiFi Not Connecting
- Verify SSID and password in `sdkconfig.defaults`
- Check 2.4GHz WiFi availability (ESP32-S3 doesn't support 5GHz)
- Review serial monitor for WiFi error messages

### NTP Clients Not Syncing
- Verify ESP32 has valid GPS fix
- Check firewall rules allow UDP port 123
- Ensure ESP32 and clients are on same network
- Test with `ntpdate -d <ESP32_IP>` for detailed diagnostics

### Display Not Working
- Check I2C connections (SDA, SCL, Power, Ground)
- Verify I2C address (default 0x3C)
- Check I2C pull-up resistors (usually built into OLED module)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Acknowledgments

- Espressif Systems for ESP-IDF framework
- NTP protocol reference: RFC 5905
- GPS NMEA protocol specification
- SSD1306 OLED driver community implementations

## References

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [NTP Protocol RFC 5905](https://tools.ietf.org/html/rfc5905)
- [NMEA Protocol](https://www.gpsinformation.org/dale/nmea.htm)
- [PlatformIO Documentation](https://docs.platformio.org/)
