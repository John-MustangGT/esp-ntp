# Quick Start Guide

Get your ESP32-S3 GPS NTP Server up and running in minutes!

## Prerequisites

Before you begin, ensure you have:

- [ ] All hardware components assembled (see [HARDWARE.md](HARDWARE.md))
- [ ] USB-C cable
- [ ] WiFi network with 2.4GHz support
- [ ] Computer with PlatformIO or ESP-IDF installed

## Step 1: Hardware Setup

1. **Connect GPS Module** to ESP32-S3:
   - GPS TX → GPIO 44
   - GPS RX → GPIO 43
   - GPS PPS → GPIO 1
   - GPS VCC → 3.3V
   - GPS GND → GND

2. **Connect OLED Display** to ESP32-S3:
   - OLED SDA → GPIO 5
   - OLED SCL → GPIO 6
   - OLED VCC → 3.3V
   - OLED GND → GND

3. **Attach GPS Antenna** to GPS module

## Step 2: Software Setup

### Option A: Using PlatformIO (Recommended)

1. **Install PlatformIO**:
   - Download [VS Code](https://code.visualstudio.com/)
   - Install PlatformIO IDE extension

2. **Clone and Open Project**:
   ```bash
   git clone https://github.com/John-MustangGT/esp-ntp.git
   cd esp-ntp
   code .  # Opens in VS Code
   ```

3. **Configure WiFi**:
   Edit `sdkconfig.defaults`:
   ```
   CONFIG_WIFI_SSID="YourNetworkName"
   CONFIG_WIFI_PASSWORD="YourPassword"
   ```

4. **Build and Upload**:
   - Connect ESP32-S3 via USB-C
   - Click "PlatformIO: Upload" (→ icon)
   - Wait for build and upload to complete

### Option B: Using ESP-IDF

1. **Install ESP-IDF**:
   ```bash
   mkdir -p ~/esp
   cd ~/esp
   git clone -b v5.1 --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ./install.sh esp32s3
   ```

2. **Setup Environment**:
   ```bash
   . ~/esp/esp-idf/export.sh
   ```

3. **Configure Project**:
   ```bash
   cd /path/to/esp-ntp
   idf.py menuconfig
   ```
   Navigate to "WiFi Configuration" and set SSID/Password

4. **Build and Flash**:
   ```bash
   idf.py build
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Step 3: First Boot

1. **Power On**: Connect USB-C cable

2. **Monitor Serial Output** (optional but recommended):
   ```bash
   pio device monitor
   # or
   idf.py monitor
   ```

3. **Watch OLED Display**:
   - Line 1: "GPS NTP Server"
   - Line 2: "Starting..."
   - Line 3: "GPS: Init"
   - Line 4: "WiFi: Connected"

## Step 4: Wait for GPS Fix

1. **Place antenna** with clear view of sky (near window or outside)

2. **Wait for GPS lock** (2-15 minutes for first fix):
   - OLED Line 2 will show "Waiting..."
   - When locked, shows UTC time (e.g., "12:34:56 UTC")
   - OLED Line 3 shows "GPS: Fix 8sat" (number varies)

3. **Check Serial Monitor**:
   ```
   I (12345) GPS: GPS driver initialized
   I (13456) MAIN: GPS fix acquired: 8 satellites
   ```

## Step 5: Find Your NTP Server IP

**Method 1: Check Serial Monitor**
```
I (23456) MAIN: got ip:192.168.1.100
```

**Method 2: Check Router**
- Log into your router
- Look for "XIAO_ESP32S3" or similar device name
- Note the IP address

**Method 3: OLED Display** (if implemented in your version)
- May show IP on one of the lines

## Step 6: Test NTP Server

From any computer on the same network:

### Linux/macOS:
```bash
ntpdate -q 192.168.1.100
```

Expected output:
```
server 192.168.1.100, stratum 1, offset -0.000123, delay 0.00234
```

### Windows PowerShell:
```powershell
w32tm /stripchart /computer:192.168.1.100 /samples:5
```

## Step 7: Configure Your Devices

### Linux/Unix Systems

Edit `/etc/ntp.conf`:
```
server 192.168.1.100 prefer iburst
```

Restart NTP:
```bash
sudo systemctl restart ntp
```

### Windows

1. Open "Date & Time" settings
2. Click "Add servers"
3. Add: `192.168.1.100`
4. Click "Sync now"

### Router (Recommended)

Configure DHCP to provide NTP server:
1. Log into router admin panel
2. Find DHCP settings
3. Set NTP server: `192.168.1.100`
4. Save and reboot router

All network devices will now automatically use your GPS NTP server!

## Verification

### Check NTP Status

**Linux/macOS**:
```bash
ntpq -p
```

Look for your ESP32 IP with:
- Stratum: 1
- Offset: < 10ms

**Windows**:
```powershell
w32tm /query /status
```

### Check GPS Status

Serial monitor should show:
```
I (45678) GPS: GPS fix valid, 8 satellites
I (45678) GPS: Time: 12:34:56 UTC
I (45678) NTP: Request from 192.168.1.50
```

### Check OLED Display

Should show:
```
GPS NTP Server
12:34:56 UTC
GPS: Fix 8sat
NTP Req: 42
```

## Troubleshooting

### No GPS Fix

- **Move antenna** to window or outdoors
- **Wait longer** - first fix can take 15 minutes
- **Check connections** - verify TX/RX are crossed
- **Check serial** - should see NMEA sentences like `$GPGGA,...`

### WiFi Not Connecting

- **Verify SSID/Password** in `sdkconfig.defaults`
- **Check WiFi band** - ESP32 only supports 2.4GHz
- **Check serial monitor** for error messages

### NTP Not Responding

- **Verify GPS fix** - OLED must show "GPS: Fix"
- **Check IP address** - ping the ESP32
- **Check firewall** - allow UDP port 123
- **Verify network** - client and ESP32 on same network

### OLED Blank

- **Check power** - measure 3.3V at OLED VCC
- **Check I2C** - verify SDA/SCL connections
- **Check I2C address** - try 0x3C or 0x3D

## Next Steps

- **Monitor Performance**: Use `ntpq -p` or similar tools
- **Set Static IP**: Configure your router for DHCP reservation
- **Add Backup**: Configure secondary NTP servers as fallback
- **Enclosure**: Build or buy a case for protection
- **Outdoor Antenna**: For better GPS reception

## Getting Help

- **Documentation**: See [README.md](README.md) for details
- **Hardware**: See [HARDWARE.md](HARDWARE.md) for assembly
- **Issues**: Report bugs on GitHub Issues
- **Community**: Join discussions on GitHub

## Success!

You now have a working Stratum 1 NTP server! Your network devices can enjoy GPS-synchronized time with microsecond precision.

**Tip**: Add a UPS or battery backup to keep your time server running during power outages.
