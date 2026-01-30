# Testing Guide

This guide explains how to test the ESP32-S3 GPS NTP Server functionality.

## Pre-Deployment Testing

Before deploying the NTP server on your network, verify basic functionality:

### 1. Hardware Verification

**Power Test**
```
Expected: OLED displays "GPS NTP Server Starting..."
If not: Check USB power, OLED connections
```

**GPS UART Test**
```bash
# Connect to serial monitor
pio device monitor
# or
idf.py monitor

Expected output:
I (1234) GPS: GPS driver initialized
I (2345) GPS: $GPGGA,123519,4807.038,N,...

If not: Check GPIO 43/44 connections, GPS power
```

**WiFi Test**
```
Expected in serial monitor:
I (3456) MAIN: connected to ap SSID:YourSSID
I (3456) MAIN: got ip:192.168.1.100

If not: Verify SSID/password in sdkconfig.defaults
```

**GPS Fix Test**
```
Wait 2-15 minutes outdoors or near window
Expected: OLED shows "GPS: Fix 8sat" (number varies)

If not: Move antenna to better location, wait longer
```

### 2. NTP Functionality Test

**Basic Query Test**
```bash
# From another computer on same network
ntpdate -q 192.168.1.100

Expected output:
server 192.168.1.100, stratum 1, offset -0.001234, delay 0.00234

If "no server suitable for synchronization found":
- Verify GPS has fix
- Check firewall allows UDP port 123
- Ping ESP32 to verify network connectivity
```

**Detailed Query Test**
```bash
# Linux/macOS
ntpdate -d 192.168.1.100

# Windows PowerShell
w32tm /stripchart /computer:192.168.1.100 /samples:10

Expected: Shows time offset and round-trip delay
```

## Production Testing

### 3. Accuracy Verification

**Compare with Reference**
```bash
# Install ntpd on a test machine
sudo apt-get install ntp ntpdate

# Sync to public NTP servers first
sudo ntpdate pool.ntp.org

# Configure to use ESP32 NTP server
sudo nano /etc/ntp.conf
# Add line: server 192.168.1.100 prefer iburst

# Restart NTP
sudo systemctl restart ntp

# Wait 5 minutes, then check
ntpq -p

Expected:
     remote           refid      st t when poll reach   delay   offset  jitter
==============================================================================
*192.168.1.100   .GPS.            1 u   25   64  377    1.234   -0.123   0.045
 pool.ntp.org    .POOL.          16 p    -   64    0    0.000    0.000   0.000
```

Interpretation:
- `*` means selected as primary source
- `st 1` confirms Stratum 1
- `delay` should be <10ms on local network
- `offset` should be <5ms (ideally <1ms)
- `jitter` should be <1ms

**Accuracy Test with chrony**
```bash
# Install chrony (more accurate than ntpd)
sudo apt-get install chrony

# Configure
sudo nano /etc/chrony/chrony.conf
# Add: server 192.168.1.100 iburst prefer

# Restart
sudo systemctl restart chrony

# Monitor
watch chronyc sources -v
watch chronyc tracking

Expected offset: <1ms
Expected jitter: <100µs
```

### 4. Load Testing

**Simultaneous Client Test**
```bash
# From multiple machines simultaneously
for i in {1..10}; do
    ntpdate -q 192.168.1.100 &
done
wait

Expected: All queries succeed
Monitor ESP32 serial output for errors
```

**Sustained Load Test**
```bash
# Continuous queries for 10 minutes
for i in {1..600}; do
    ntpdate -q 192.168.1.100
    sleep 1
done

Expected: 
- All queries succeed
- ESP32 doesn't crash or restart
- Accuracy remains stable
```

### 5. Stability Testing

**Long-Term Stability**
```bash
# Set up monitoring on a Linux machine
cat > ntp_monitor.sh << 'SCRIPT'
#!/bin/bash
while true; do
    date >> /var/log/ntp_test.log
    ntpdate -q 192.168.1.100 >> /var/log/ntp_test.log 2>&1
    sleep 300  # Every 5 minutes
done
SCRIPT

chmod +x ntp_monitor.sh
nohup ./ntp_monitor.sh &

# Let run for 24 hours, then analyze
grep "offset" /var/log/ntp_test.log | awk '{print $10}' | \
    awk '{sum+=$1; sumsq+=$1*$1} END {
        print "Mean:", sum/NR;
        print "StdDev:", sqrt(sumsq/NR - (sum/NR)^2)
    }'

Expected:
- Mean offset: <1ms
- Standard deviation: <0.5ms
- No crashes or gaps in log
```

**Power Cycle Test**
```
1. Unplug ESP32
2. Wait 10 seconds
3. Plug back in
4. Monitor serial output
5. Verify GPS fix within 1-2 minutes (warm start)
6. Verify NTP service resumes

Repeat 10 times
Expected: All restarts successful, no issues
```

**Network Failure Recovery**
```
1. Disconnect ESP32 from WiFi (turn off router)
2. Wait 60 seconds
3. Reconnect WiFi
4. Monitor serial output

Expected:
- "WiFi disconnected" message
- Automatic reconnection attempt
- "WiFi connected" message
- NTP service resumes
```

### 6. GPS Testing

**GPS Accuracy Test**
```
Compare GPS time with authoritative source:

1. Visit time.gov or time.is in browser
2. Note the exact time shown
3. Check OLED display on ESP32
4. Compare times

Expected difference: <1 second
(NTP clients will have microsecond accuracy, 
 but display shows only whole seconds)
```

**GPS Signal Quality Test**
```
# Monitor GPS fix in various locations
# Record satellite count and fix status

Location          | Satellites | Fix Time
------------------|------------|----------
Outdoor, clear sky| 8-12       | 30-90s
Window, indoors   | 4-8        | 60-300s
Inside, no window | 0-4        | No fix

Expected: Better locations = more satellites = faster fix
```

**PPS Signal Test**
```
# Check if PPS is working
# In serial monitor, look for:
I (12345) GPS: PPS timestamp: 12345678901

# Verify timestamp increments by ~1000000 each second
# (1 million microseconds = 1 second)

Expected: Regular PPS updates when GPS has fix
```

## Automated Testing

### 7. Python Test Script

```python
#!/usr/bin/env python3
import ntplib
import time
import statistics

def test_ntp_server(server_ip, num_samples=30):
    client = ntplib.NTPClient()
    offsets = []
    delays = []
    
    print(f"Testing NTP server at {server_ip}")
    print(f"Taking {num_samples} samples...")
    
    for i in range(num_samples):
        try:
            response = client.request(server_ip, version=4, timeout=2)
            offsets.append(response.offset * 1000)  # Convert to ms
            delays.append(response.delay * 1000)
            print(f"Sample {i+1}: offset={response.offset*1000:.3f}ms, "
                  f"delay={response.delay*1000:.3f}ms, stratum={response.stratum}")
            time.sleep(2)
        except Exception as e:
            print(f"Error: {e}")
    
    if offsets:
        print("\nResults:")
        print(f"Mean offset: {statistics.mean(offsets):.3f}ms")
        print(f"Std dev offset: {statistics.stdev(offsets):.3f}ms")
        print(f"Mean delay: {statistics.mean(delays):.3f}ms")
        print(f"Min offset: {min(offsets):.3f}ms")
        print(f"Max offset: {max(offsets):.3f}ms")
        
        # Pass criteria
        if statistics.mean(offsets) < 5.0 and statistics.stdev(offsets) < 2.0:
            print("\n✓ PASS: NTP server accuracy is good")
        else:
            print("\n✗ FAIL: NTP server accuracy is poor")

if __name__ == "__main__":
    test_ntp_server("192.168.1.100")  # Change to your ESP32 IP
```

Save as `test_ntp.py` and run:
```bash
pip3 install ntplib
python3 test_ntp.py
```

## Performance Benchmarks

### Expected Performance Metrics

| Metric | Target | Acceptable | Poor |
|--------|--------|------------|------|
| GPS Fix Time (Cold) | <60s | <120s | >120s |
| GPS Fix Time (Warm) | <10s | <30s | >30s |
| GPS Satellites | >8 | 6-8 | <6 |
| NTP Offset | <1ms | <5ms | >5ms |
| NTP Jitter | <0.1ms | <1ms | >1ms |
| NTP Delay | <2ms | <10ms | >10ms |
| WiFi Reconnect | <10s | <30s | >30s |
| Uptime | >7 days | >1 day | <1 day |

## Troubleshooting Tests

### If NTP Not Responding

**Test 1: Check if GPS has fix**
```
Look at OLED or serial monitor
Expected: "GPS: Fix Xsat"
If not: GPS issue, not NTP issue
```

**Test 2: Check if NTP port is open**
```bash
# From client machine
nmap -sU -p 123 192.168.1.100

Expected: 123/udp open
If "filtered": Firewall blocking
If "closed": NTP server not running
```

**Test 3: Check network connectivity**
```bash
ping 192.168.1.100

Expected: Replies from ESP32
If timeout: Network issue
```

**Test 4: Check with tcpdump**
```bash
# On client machine
sudo tcpdump -i any port 123

# In another terminal
ntpdate -q 192.168.1.100

Expected: See NTP request and response packets
If only request: ESP32 not responding
```

### If GPS Not Getting Fix

**Test 1: Check NMEA sentences**
```
Serial monitor should show:
$GPGGA,...
$GPRMC,...

If not: UART connection problem
If yes but no fix: Signal issue
```

**Test 2: Check GPS antenna**
```
Verify:
- Antenna is connected
- Antenna has clear sky view
- No metal objects blocking
```

**Test 3: Try extended wait**
```
Wait 15 minutes outdoors
If still no fix: Possible GPS module defect
```

## Certification Testing

For use in production environments:

1. **Accuracy Certification**: Compare against NIST time source for 7 days
2. **Stability Certification**: Continuous operation for 30 days
3. **Load Certification**: 1000 requests/minute for 24 hours
4. **Failover Certification**: Network failures, power cycles, GPS signal loss

## Validation Checklist

- [ ] Hardware connections verified
- [ ] GPS gets fix outdoors
- [ ] NTP responds to queries
- [ ] Stratum 1 confirmed
- [ ] Accuracy <5ms offset
- [ ] Jitter <1ms
- [ ] Survives power cycle
- [ ] Survives network disconnect
- [ ] Multiple clients supported
- [ ] 24-hour stability test passed
- [ ] Display shows correct information
- [ ] Serial logs show no errors

## Documentation of Results

Keep a test log:
```
Date: YYYY-MM-DD
Tester: Your Name
Hardware: ESP32-S3 + ATGM336M + OLED
Location: Indoor/Outdoor
Tests Performed: [List]
Results: [Pass/Fail]
Issues Found: [List]
Notes: [Any observations]
```

## Support

If tests fail:
1. Review [QUICKSTART.md](QUICKSTART.md)
2. Check [HARDWARE.md](HARDWARE.md) for wiring
3. See [CONFIGURATION.md](CONFIGURATION.md) for settings
4. Open GitHub issue with test results
