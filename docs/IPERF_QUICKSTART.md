# Quick Start: iPerf Server

## What was added?

BeaconBit now includes a built-in iPerf server for network performance testing!

## How to use it

### 1. Build and flash your ESP32

```bash
# Source ESP-IDF (adjust path to your installation)
source $HOME/esp/esp-idf/export.sh

# Build and flash
idf.py build flash monitor
```

### 2. Connect to your ESP32 Access Point

- **SSID**: Look for it in the serial monitor output (e.g., `BeaconBit-XXXX`)
- **Password**: As configured in your device

### 3. Run iPerf from your client

**From Linux/macOS:**

```bash
# Install iperf if needed
# Ubuntu/Debian: sudo apt install iperf
# macOS: brew install iperf

# Run test
iperf -c 192.168.4.1 -i 1 -t 30
```

**From Windows:**

```cmd
# Download from https://iperf.fr/iperf-download.php

# Run test
iperf.exe -c 192.168.4.1 -i 1 -t 30
```

## Configuration (optional)

To change settings:

```bash
idf.py menuconfig
# Navigate to: Example Configuration
# - Enable iPerf Server: Yes/No
# - iPerf Server Port: 5001 (default)
```

## Disable iPerf Server

If you don't need it:

```bash
idf.py menuconfig
# Example Configuration → Enable iPerf Server → No
idf.py build flash
```

## More Information

- Full documentation: `docs/IPERF.md`
- Implementation details: `docs/IPERF_IMPLEMENTATION.md`

## Quick Troubleshooting

**Can't connect?**

- Make sure you're connected to the ESP32's WiFi
- Ping 192.168.4.1 to verify connectivity
- Check that port 5001 is not blocked

**Low performance?**

- Move closer to the ESP32
- Try a different WiFi channel
- Reduce number of connected clients

## Example Output

```plaintext
I (12345) beaconbit: iPerf server started on port 5001
I (12346) beaconbit: Run 'iperf -c 192.168.4.1 -i 1 -t 30' from a connected client to test
```

Then from your client:

```bash
------------------------------------------------------------
Client connecting to 192.168.4.1, TCP port 5001
TCP window size: 85.0 KByte (default)
------------------------------------------------------------
[  3] local 192.168.4.2 port 54321 connected with 192.168.4.1 port 5001
[ ID] Interval       Transfer     Bandwidth
[  3]  0.0- 1.0 sec  5.12 MBytes  42.9 Mbits/sec
[  3]  1.0- 2.0 sec  5.25 MBytes  44.0 Mbits/sec
...
```
