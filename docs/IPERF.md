# iPerf Server Configuration

## Overview

BeaconBit includes an integrated iPerf server for network performance testing. This allows you to measure throughput, bandwidth, and network quality of your ESP32 Access Point.

## Features

- **Automatic Startup**: iPerf server starts automatically when the Access Point initializes
- **Configurable Port**: Default port 5001 (standard iPerf port), configurable via menuconfig
- **IPv4 Support**: Currently supports IPv4 connections only
- **Multi-target Compatible**: Works across all supported ESP32 targets (esp32, esp32c3, esp32s3, esp32c6)

## Configuration

### Enable/Disable iPerf Server

The iPerf server can be enabled or disabled at build time using `idf.py menuconfig`:

```bash
idf.py menuconfig
# Navigate to: Example Configuration → Enable iPerf Server
```

Configuration options:

- **BEACONBIT_IPERF_SERVER_ENABLE**: Enable/disable iPerf server (default: enabled)
- **BEACONBIT_IPERF_SERVER_PORT**: Server port number (default: 5001, range: 1024-65535)

### Build-time Configuration

To disable the iPerf server at build time, you can also edit `sdkconfig`:

```plaintext
CONFIG_BEACONBIT_IPERF_SERVER_ENABLE=n
```

Or keep it enabled with a custom port:

```plaintext
CONFIG_BEACONBIT_IPERF_SERVER_ENABLE=y
CONFIG_BEACONBIT_IPERF_SERVER_PORT=5201
```

## Usage

### Testing from a Connected Client

1. **Connect to the Access Point**
   - SSID: `BeaconBit-XXXX` (where XXXX is from your device's MAC address)
   - Password: As configured in your NVS settings

2. **Run iPerf Client Test**
   **Linux/macOS:**

   ```bash
   # Install iperf (if not already installed)
   # Ubuntu/Debian: sudo apt install iperf
   # macOS: brew install iperf
   
   # Run TCP throughput test (default)
   iperf -c 192.168.4.1 -i 1 -t 30
   
   # Run UDP throughput test
   iperf -c 192.168.4.1 -u -b 50M -i 1 -t 30
   ```

   **Windows:**

   ```cmd
   # Download iperf from https://iperf.fr/iperf-download.php
   
   # Run TCP throughput test
   iperf.exe -c 192.168.4.1 -i 1 -t 30
   
   # Run UDP throughput test
   iperf.exe -c 192.168.4.1 -u -b 50M -i 1 -t 30
   ```

### Command Parameters Explained

- `-c 192.168.4.1`: Connect to server at IP 192.168.4.1 (ESP32's default AP IP)
- `-i 1`: Report interval every 1 second
- `-t 30`: Run test for 30 seconds
- `-u`: Use UDP instead of TCP
- `-b 50M`: Target bandwidth of 50 Mbps (UDP only)
- `-p 5001`: Server port (use if you changed the default)

### Example Output

**Client-side (iPerf client):**

```plaintext
------------------------------------------------------------
Client connecting to 192.168.4.1, TCP port 5001
TCP window size: 85.0 KByte (default)
------------------------------------------------------------
[  3] local 192.168.4.2 port 54321 connected with 192.168.4.1 port 5001
[ ID] Interval       Transfer     Bandwidth
[  3]  0.0- 1.0 sec  5.12 MBytes  42.9 Mbits/sec
[  3]  1.0- 2.0 sec  5.25 MBytes  44.0 Mbits/sec
[  3]  2.0- 3.0 sec  5.18 MBytes  43.4 Mbits/sec
...
```

**Server-side (ESP32 serial monitor):**

```plaintext 
I (12345) beaconbit: iPerf server started on port 5001
I (12346) beaconbit: Run 'iperf -c 192.168.4.1 -i 1 -t 30' from a connected client to test
I (15678) iperf: mode=tcp-server sip=192.168.4.1:5001, dip=192.168.4.2:54321, interval=3
I (18678) iperf: Interval: 0-3 sec, Bandwidth: 43.4 Mbits/sec
```

## Performance Expectations

### Typical Throughput Values

Performance varies by ESP32 chip and WiFi configuration:

- **ESP32**: 20-40 Mbps (TCP), 30-50 Mbps (UDP)
- **ESP32-C3**: 30-50 Mbps (TCP), 40-60 Mbps (UDP)
- **ESP32-S3**: 40-60 Mbps (TCP), 50-70 Mbps (UDP)
- **ESP32-C6**: 50-80 Mbps (TCP), 60-90 Mbps (UDP) with WiFi 6

### Factors Affecting Performance

- **Channel Bandwidth**: HT20 (20 MHz) vs HT40 (40 MHz)
- **Authentication Mode**: WPA2 vs WPA3 (WPA3 has slightly higher overhead)
- **Number of Connected Clients**: More clients = lower per-client throughput
- **Distance and Interference**: Signal strength and RF environment
- **ESP32 Target**: Newer chips (C6, S3) have better WiFi performance

## Troubleshooting

### iPerf Server Not Starting

**Symptom:** No log message about iPerf server starting

**Solutions:**

1. Check that `CONFIG_BEACONBIT_IPERF_SERVER_ENABLE=y` in sdkconfig
2. Verify the iperf component was downloaded: check `managed_components/espressif__iperf/`
3. Run `idf.py fullclean && idf.py build` to ensure clean build

### Connection Refused

**Symptom:** `connect failed: Connection refused`

**Solutions:**

1. Verify you're connected to the ESP32 Access Point
2. Confirm the IP address is `192.168.4.1`
3. Check the port matches your configuration (default: 5001)
4. Ensure WiFi is fully initialized (wait a few seconds after boot)

### Low Throughput

**Symptom:** Performance much lower than expected

**Solutions:**

1. Move client closer to the ESP32
2. Change WiFi channel to avoid interference (see `docs/CHANNEL_SELECTION.md`)
3. Use HT40 bandwidth if supported (menuconfig → Component config → Wi-Fi)
4. Reduce number of connected clients
5. Check for RF interference from other devices

### Out of Memory

**Symptom:** `Failed to start iPerf server: ESP_ERR_NO_MEM`

**Solutions:**

1. Reduce maximum STA connections (`CONFIG_ESP_MAX_STA_CONN`)
2. Increase heap size in menuconfig if possible
3. Disable other features temporarily to free up memory

## Integration Notes

### Memory Usage

The iPerf server component requires approximately:

- **Code size**: ~40-60 KB flash
- **Runtime RAM**: ~10-20 KB heap
- **Stack size**: 4 KB per connection

### Task Priority

The iPerf server runs in a separate FreeRTOS task with default priority. It should not interfere with normal WiFi operations or the web server.

### Compatibility

- **iPerf Version**: Compatible with iPerf 2.x clients (not iPerf3)
- **Protocol**: TCP and UDP
- **IP Version**: IPv4 only (IPv6 support not yet implemented)

## Advanced Usage

### Bidirectional Testing

Test both upload and download speeds:

```bash
# Test upload (client → ESP32)
iperf -c 192.168.4.1 -i 1 -t 30

# Test download (ESP32 → client)
iperf -c 192.168.4.1 -i 1 -t 30 -r
```

### Parallel Streams

Test with multiple simultaneous connections:

```bash
# 4 parallel TCP streams
iperf -c 192.168.4.1 -i 1 -t 30 -P 4
```

### Different Window Sizes

Adjust TCP window size for testing:

```bash
# 128KB window size
iperf -c 192.168.4.1 -i 1 -t 30 -w 128K
```

## See Also

- [ESP-IDF iPerf Component Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_netif.html)
- [iPerf 2 Documentation](https://iperf.fr/)
- [CHANNEL_SELECTION.md](CHANNEL_SELECTION.md) - Optimize WiFi channel for better performance
- [MULTI_TARGET.md](MULTI_TARGET.md) - Performance differences between ESP32 targets
