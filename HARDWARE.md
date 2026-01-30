# Hardware Assembly Guide

This guide provides detailed instructions for assembling the ESP32-S3 GPS NTP Server.

## Bill of Materials (BOM)

| Component | Quantity | Notes |
|-----------|----------|-------|
| Seeed Studios Xiao ESP32-S3 | 1 | Main microcontroller |
| Teyleten ATGM336M GPS Module | 1 | GPS receiver with PPS output |
| 0.91" 128x32 OLED Display (SSD1306) | 1 | I2C interface |
| GPS Antenna (Passive or Active) | 1 | Compatible with ATGM336M |
| Jumper Wires | ~10 | For connections |
| USB-C Cable | 1 | For programming and power |
| Breadboard or PCB | 1 | Optional for prototyping |

## Wiring Diagram

### Seeed Xiao ESP32-S3 to ATGM336M GPS Module

| ESP32-S3 Pin | Function | GPS Module Pin | Notes |
|--------------|----------|----------------|-------|
| GPIO 43 | UART TX | RX | Serial data to GPS |
| GPIO 44 | UART RX | TX | Serial data from GPS |
| GPIO 1 | PPS Input | PPS | Pulse per second output |
| 3.3V | Power | VCC | Power supply |
| GND | Ground | GND | Common ground |

### Seeed Xiao ESP32-S3 to OLED Display

| ESP32-S3 Pin | Function | OLED Pin | Notes |
|--------------|----------|----------|-------|
| GPIO 5 | I2C SDA | SDA | I2C data line |
| GPIO 6 | I2C SCL | SCL | I2C clock line |
| 3.3V | Power | VCC | Power supply |
| GND | Ground | GND | Common ground |

## Assembly Steps

### 1. Prepare the Components

- Ensure all components are functional
- Check GPS module has antenna connected
- Verify OLED display I2C address (default 0x3C)

### 2. Connect the GPS Module

1. Connect GPS VCC to ESP32-S3 3.3V
2. Connect GPS GND to ESP32-S3 GND
3. Connect GPS TX to ESP32-S3 GPIO 44 (RX)
4. Connect GPS RX to ESP32-S3 GPIO 43 (TX)
5. Connect GPS PPS to ESP32-S3 GPIO 1

**Note**: The PPS signal is critical for accurate time synchronization. Ensure this connection is secure.

### 3. Connect the OLED Display

1. Connect OLED VCC to ESP32-S3 3.3V (can share with GPS)
2. Connect OLED GND to ESP32-S3 GND (can share with GPS)
3. Connect OLED SDA to ESP32-S3 GPIO 5
4. Connect OLED SCL to ESP32-S3 GPIO 6

**Note**: Most I2C OLED modules have built-in pull-up resistors. If yours doesn't, add 4.7kΩ pull-ups on SDA and SCL.

### 4. Attach GPS Antenna

- Connect the GPS antenna to the ATGM336M module
- For best performance, use an active antenna with clear sky view
- Place antenna near a window or outside for optimal signal reception

### 5. Power Connection

- Connect USB-C cable to the Xiao ESP32-S3
- Power can be from:
  - Computer USB port (for development)
  - USB power adapter (5V, 500mA minimum)
  - Power bank (for portable operation)

## Testing the Assembly

### 1. Visual Inspection

- Check all connections are secure
- Verify no short circuits between power and ground
- Ensure antenna is properly connected to GPS module

### 2. Power On Test

1. Connect USB-C cable
2. OLED should light up
3. Check for "GPS NTP Server Starting..." message

### 3. GPS Fix Test

1. Place antenna with clear view of sky
2. Wait 2-15 minutes for initial GPS fix
3. OLED should show "GPS: Fix Xsat" when locked
4. Serial monitor should show NMEA sentences

### 4. WiFi Connection Test

1. Verify WiFi credentials are configured
2. Check OLED shows "WiFi: Connected"
3. Note the IP address from serial monitor

### 5. NTP Server Test

From another device on the same network:

```bash
ntpdate -q <ESP32_IP_ADDRESS>
```

Expected output:
```
server <ESP32_IP>, stratum 1, offset X.XXXXXX, delay X.XXXXX
```

## Troubleshooting

### GPS Module Issues

**Symptom**: No GPS data
- **Check**: UART TX/RX connections (they should be crossed)
- **Check**: GPS module power (3.3V between VCC and GND)
- **Check**: Antenna connection
- **Check**: Serial monitor for NMEA sentences

**Symptom**: GPS data but no fix
- **Move**: Antenna to location with clear sky view
- **Wait**: Initial fix can take 15 minutes
- **Check**: Antenna is correct type for module

**Symptom**: No PPS signal
- **Check**: PPS pin connection to GPIO 1
- **Check**: GPS has valid fix (PPS only active with fix)
- **Verify**: PPS LED on GPS module blinks once per second

### OLED Display Issues

**Symptom**: Blank display
- **Check**: Power connections (3.3V and GND)
- **Check**: I2C connections (SDA and SCL)
- **Verify**: I2C address (use I2C scanner)
- **Try**: Pull-up resistors on SDA/SCL if not built-in

**Symptom**: Corrupted display
- **Check**: Wire length (keep I2C wires short)
- **Reduce**: I2C clock speed in code if needed
- **Add**: Decoupling capacitor near display

### Power Issues

**Symptom**: Device resets randomly
- **Use**: Higher current power supply (1A recommended)
- **Add**: Decoupling capacitors (10µF and 100nF)
- **Check**: USB cable quality

## Advanced Setup

### Permanent Installation

For permanent deployment:

1. **Use a PCB**: Design custom PCB for cleaner assembly
2. **Enclosure**: 3D print or purchase enclosure
3. **Weatherproofing**: If outdoor, use weatherproof enclosure
4. **External Antenna**: Use antenna extension cable
5. **PoE**: Consider PoE module for single-cable solution

### Performance Optimization

1. **Antenna Placement**: Roof-mounted for best sky view
2. **Grounding**: Proper grounding reduces noise
3. **Power Supply**: Use clean, regulated power supply
4. **Cable Quality**: Use shielded cables for GPS in noisy environments

## Safety Notes

- Use only 3.3V or 5V power sources
- Do not connect directly to mains power
- Ensure proper grounding when using outdoor antennas
- Follow local regulations for RF devices
- Keep away from moisture unless properly enclosed

## Maintenance

- **Regular**: Check GPS antenna connection
- **Monthly**: Verify NTP accuracy against reference
- **As needed**: Clean OLED display
- **Seasonal**: Check outdoor antenna weatherproofing

## Support

For hardware-related issues:
- Consult component datasheets
- Check ESP32-S3 documentation
- Visit project GitHub issues page
