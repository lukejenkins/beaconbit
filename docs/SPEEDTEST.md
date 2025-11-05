# Web-Based Speed Test

## Overview

BeaconBit includes a comprehensive web-based speed testing solution that allows you to measure WiFi performance directly from your browser. No additional software installation is required for basic testing, while power users can leverage the built-in iperf server for more accurate measurements.

## Features

### Browser-Based Testing (Quick Test)

- **Download Test**: Streams data for 10 seconds with real-time Mbps display
- **Upload Test**: Uploads 5MB of data and returns instant results
- **Real-time Progress**: Animated progress bars showing current speed
- **Results History**: Previous test results displayed on the page
- **No Installation Required**: Works on any device with a modern web browser

### iperf Integration (Advanced)

- **Status Display**: Shows iperf server running state with visual indicator
- **Port Information**: Displays the configured port (default: 5001)
- **One-Click Copy**: Copy the iperf command with a single click
- **Platform-Specific Instructions**: Installation guides for Linux, macOS, and Windows

## Accessing the Speed Test

1. **Connect to your ESP32 Access Point**
   - SSID: As shown in serial monitor (e.g., `BeaconBit-XXXX`)
   - Password: As configured in your device

2. **Navigate to the speed test page**
   - Direct URL: `http://192.168.4.1/speedtest`
   - Or click "⚡ Speed Test" link from the main configuration page

## Using the Quick Test

### Download Test

1. Click the **"Download Test"** button
2. Watch the progress bar fill as data is downloaded
3. See real-time speed updates during the test
4. View final results with average speed and duration

**How it works:**
- ESP32 streams 8KB chunks of data to your browser for 10 seconds
- JavaScript measures bytes received and calculates Mbps
- Results are displayed immediately after completion

### Upload Test

1. Click the **"Upload Test"** button
2. Browser generates 5MB of test data
3. Data is uploaded to the ESP32
4. Results show speed and duration

**How it works:**
- Browser creates a 5MB Uint8Array with pattern data
- Data is POSTed to `/api/speedtest/upload`
- ESP32 measures reception speed and returns JSON results

## Using iperf (Advanced)

For more accurate bandwidth testing, use the built-in iperf server:

### 1. Switch to Advanced Tab

Click the **"Advanced (iperf)"** tab in the speed test interface.

### 2. Install iperf Client

**Linux:**
```bash
sudo apt install iperf
```

**macOS:**
```bash
brew install iperf
```

**Windows:**
Download from [iperf.fr](https://iperf.fr/iperf-download.php)

### 3. Run Test

Copy the command shown on the page (default):
```bash
iperf -c 192.168.4.1 -i 1 -t 30
```

Or customize with different parameters:
```bash
# Different test duration
iperf -c 192.168.4.1 -i 1 -t 60

# UDP test instead of TCP
iperf -c 192.168.4.1 -u -b 50M -i 1 -t 30

# Bidirectional test
iperf -c 192.168.4.1 -i 1 -t 30 -d
```

## Expected Performance

Performance varies based on ESP32 chip variant and configuration:

| Device | Browser Test | iperf Test |
|--------|--------------|------------|
| ESP32 (classic) | 15-35 Mbps | 20-40 Mbps |
| ESP32-C3 | 25-45 Mbps | 30-50 Mbps |
| ESP32-S3 | 35-55 Mbps | 40-60 Mbps |
| ESP32-C6 (WiFi 6) | 45-75 Mbps | 50-80 Mbps |

**Note:** Browser-based tests are typically 5-10% slower than iperf due to HTTP overhead and JavaScript processing.

## Factors Affecting Speed

### WiFi Configuration

- **Channel Bandwidth**: HT40 (40 MHz) provides ~2x speed of HT20 (20 MHz)
- **Channel Selection**: Channels 1, 6, 11 recommended (non-overlapping)
- **Authentication**: WPA3 has slightly higher overhead than WPA2

### Environmental

- **Distance**: Performance degrades with distance from ESP32
- **Interference**: Other WiFi networks, Bluetooth, microwave ovens
- **Obstacles**: Walls, furniture, metal objects reduce signal strength

### Device-Specific

- **Client Capabilities**: Older devices may not support HT40 or WiFi 6
- **Browser**: Modern browsers (Chrome, Firefox, Safari) perform best
- **Background Activity**: Other apps using network reduce available bandwidth

## Troubleshooting

### Slow Download/Upload Speeds

1. **Check distance to ESP32**
   - Move closer to the access point
   - Ensure line of sight if possible

2. **Change WiFi channel**
   - Use main config page to select channel 1, 6, or 11
   - Avoid channels used by nearby networks

3. **Check for interference**
   - Disable Bluetooth on devices
   - Move away from microwave ovens
   - Close other WiFi networks in range

### Test Fails to Start

1. **Refresh the page** - Concurrent test prevention may be active
2. **Check browser console** - Look for JavaScript errors
3. **Try different browser** - Use Chrome, Firefox, or Safari

### iperf Shows as Disabled

1. **Check build configuration**
   ```bash
   idf.py menuconfig
   # Navigate to: Example Configuration → Enable iPerf Server
   ```

2. **Rebuild and flash**
   ```bash
   idf.py build flash
   ```

### One Test at a Time

The speed test system prevents concurrent tests to ensure accurate measurements. If you see "Test already in progress", wait for the current test to complete or refresh the page.

## API Endpoints

For programmatic access:

### Download Test
```http
GET /api/speedtest/download
```
Streams binary data for 10 seconds. Measure bytes received to calculate speed.

### Upload Test
```http
POST /api/speedtest/upload
Content-Type: application/octet-stream
Body: <test data>
```
Returns JSON:
```json
{
  "bytes": 5242880,
  "duration": 13.05,
  "mbps": 3.21
}
```

### iperf Status
```http
GET /api/speedtest/iperf
```
Returns JSON:
```json
{
  "enabled": true,
  "port": 5001,
  "command": "iperf -c 192.168.4.1 -i 1 -t 30"
}
```

## Comparison: Browser vs iperf

| Feature | Browser Test | iperf |
|---------|--------------|-------|
| **Installation** | None required | Need iperf client |
| **Ease of Use** | Very easy | Moderate |
| **Accuracy** | Good (±5-10%) | Excellent |
| **Real-time Display** | Yes, in browser | Yes, in terminal |
| **Protocol Overhead** | HTTP (higher) | Raw TCP/UDP (lower) |
| **Best For** | Quick checks, mobile | Benchmarking, analysis |

## Technical Details

### Memory Usage

- **Browser test**: ~15-20KB RAM during test
- **iperf server**: ~10-20KB RAM baseline, ~4KB per connection
- **Concurrent prevention**: Single `speedtest_in_progress` flag

### Implementation

- **Backend**: C code in `main/softap_webserver.c`
- **Frontend**: Minified JavaScript in HTML response
- **Data pattern**: Sequential byte pattern (0x00-0xFF repeated)
- **Chunked transfer**: 8KB chunks for efficient streaming

### Stack Protection

Previous versions experienced stack overflows with large HTML/CSS/JavaScript strings. Current implementation uses:
- `static const` storage in flash memory
- Split HTML into 4 smaller chunks
- Minified CSS/JavaScript to reduce size
- Dynamic allocation for config page buffers

## See Also

- [IPERF.md](IPERF.md) - Detailed iperf server documentation
- [IPERF_QUICKSTART.md](IPERF_QUICKSTART.md) - Quick start guide for iperf
- Main README.md - General project information
