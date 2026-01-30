# Quick Start Guide

## Installation

### 1. Install PlatformIO

**Option A: VS Code Extension**
1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the PlatformIO IDE extension
3. Restart VS Code

**Option B: Command Line**
```bash
pip install platformio
```

### 2. Clone Repository

```bash
git clone https://github.com/John-MustangGT/esp-ntp.git
cd esp-ntp
```

### 3. Connect Hardware

- Connect XIAO ESP32-S3 via USB-C
- Connect ATGM336H GPS module:
  - GPS TX → XIAO RX (GPIO44)
  - GPS GND → XIAO GND
  - GPS VCC → XIAO 3.3V
  - GPS PPS → XIAO GPIO1
- Connect I2C OLED Display:
  - OLED SDA → XIAO SDA
  - OLED SCL → XIAO SCL
  - OLED VCC → XIAO 3.3V
  - OLED GND → XIAO GND

### 4. Build & Upload

```bash
# Build firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## First Time Setup

1. **Power on device** - It will boot into AP mode (no valid configuration)

2. **Connect to WiFi AP**
   - SSID: `ESP-NTP-Config`
   - Password: `configure`

3. **Open configuration portal**
   - Navigate to: `http://192.168.4.1`

4. **Enter your settings**
   - WiFi SSID (your home/office network)
   - WiFi Password
   - NTP Server (default: `pool.ntp.org`)
   - Timezone offset in seconds (e.g., -18000 for EST)

5. **Save and reboot**
   - Click "Save & Reboot"
   - Device will restart and connect to your WiFi

## Verify Operation

After connecting to your WiFi:

1. **Check serial monitor** for connection status
2. **Observe OLED display** for time and sync status
3. **Access web interface** at device IP (shown in serial log)
4. **Monitor GPS fix** - may take 30-60 seconds for cold start

## Troubleshooting

### Device won't connect to WiFi
- Verify SSID and password in configuration
- Check WiFi signal strength
- Erase NVS to reset: Hold reset button, upload firmware again

### GPS not getting fix
- Ensure GPS has clear view of sky
- Wait 30-60 seconds for cold start
- Check GPS module connections
- Verify GPS TX is connected to XIAO RX (GPIO44)

### PPS not working
- Verify GPIO1 connection to GPS PPS output
- Check serial monitor for PPS pulse counts
- Some GPS modules require configuration to enable PPS

### OLED display blank
- Verify I2C connections
- Check I2C address (default 0x3C, some use 0x3D)
- Ensure 3.3V power supply is stable

## Web API

Access device status via HTTP:

```bash
# Get status JSON
curl http://<device-ip>/api/status

# Example response:
{
  "version": "1.0.0",
  "device": "ESP-NTP",
  "config_valid": true
}
```

## Next Steps

- Configure timezone offset for your location
- Set up OTA updates for remote firmware updates
- Monitor time sync accuracy
- Integrate with other systems via NTP server mode (future feature)
