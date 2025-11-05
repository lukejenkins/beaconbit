# iPerf Server Integration - Implementation Summary

## Changes Made

This document summarizes the changes made to add iPerf server capabilities to BeaconBit.

### 1. Component Dependencies

#### `main/idf_component.yml`

- Added `iperf` component dependency (version >= 0.1.1)
- This pulls in the ESP-IDF iperf component from the component registry

### 2. Build Configuration

#### `main/CMakeLists.txt`

- Added `iperf` to `PRIV_REQUIRES` list
- This links the iperf library into the main component

#### `main/Kconfig.projbuild`

- Added `BEACONBIT_IPERF_SERVER_ENABLE` boolean option (default: enabled)
  - Allows enabling/disabling iPerf server at build time
- Added `BEACONBIT_IPERF_SERVER_PORT` integer option (default: 5001)
  - Configures the listening port for the iPerf server
  - Range: 1024-65535

### 3. Source Code Changes

#### `main/main.c`

**Includes:**

- Added conditional include for `iperf.h` when `CONFIG_BEACONBIT_IPERF_SERVER_ENABLE` is defined

**New Function:**

```c
static void iperf_init(void)
```

- Initializes and starts the iPerf server
- Configures server mode (IPERF_FLAG_SERVER)
- Sets IPv4 mode (IPERF_IP_TYPE_IPV4)
- Uses port from Kconfig (CONFIG_BEACONBIT_IPERF_SERVER_PORT)
- Sets default interval (3 seconds) and test time (30 seconds)
- Logs success/failure and provides usage instructions

**Modified Function:**

```c
void app_main(void)
```

- Added call to `iperf_init()` after web server initialization
- Only compiled in when `CONFIG_BEACONBIT_IPERF_SERVER_ENABLE` is defined
- Ensures WiFi and web server are fully initialized before starting iPerf

### 4. Documentation

#### `docs/IPERF.md` (New File)

Comprehensive documentation covering:

- Overview and features
- Configuration options (menuconfig and sdkconfig)
- Usage instructions for Linux, macOS, and Windows
- Client command examples (TCP and UDP)
- Expected performance benchmarks by ESP32 target
- Troubleshooting common issues
- Advanced usage examples
- Memory and compatibility notes

#### `README.md`

- Added iPerf server to "Completed Features" section
- Removed iPerf from "To-Do" roadmap
- Referenced `docs/IPERF.md` for detailed information

## How It Works

### Initialization Flow

1. `app_main()` is called on boot
2. NVS is initialized
3. WiFi SoftAP is configured and started
4. Web server is started
5. **iPerf server is started** (if enabled in config)
6. Server listens on configured port (default: 5001)

### Testing Flow

1. Client connects to ESP32 Access Point (SSID: BeaconBit-XXXX)
2. Client receives IP address (typically 192.168.4.2)
3. Client runs: `iperf -c 192.168.4.1 -i 1 -t 30`
4. iPerf tests throughput between client and ESP32
5. Results are displayed on both client and ESP32 serial monitor

## Configuration Examples

### Enable with Custom Port

```bash
idf.py menuconfig
# Navigate to: Example Configuration → Enable iPerf Server → Yes
# Navigate to: Example Configuration → iPerf Server Port → 5201
idf.py build flash
```

### Disable iPerf Server

```bash
idf.py menuconfig
# Navigate to: Example Configuration → Enable iPerf Server → No
idf.py build flash
```

### Using sdkconfig Directly

```ini
# Enable iPerf on port 5001 (default)
CONFIG_BEACONBIT_IPERF_SERVER_ENABLE=y
CONFIG_BEACONBIT_IPERF_SERVER_PORT=5001

# Or disable it
CONFIG_BEACONBIT_IPERF_SERVER_ENABLE=n
```

## Memory Impact

### Flash (Code Size)

- ~40-60 KB additional flash usage when enabled
- 0 bytes when disabled (code is conditionally compiled)

### RAM (Heap)

- ~10-20 KB heap used by iPerf server task
- ~4 KB stack per active connection
- Minimal impact when no tests are running

## Multi-Target Compatibility

The implementation is compatible with all supported ESP32 targets:

- **esp32**: Classic ESP32 with 802.11 b/g/n
- **esp32c3**: RISC-V based, 802.11 b/g/n
- **esp32s3**: Dual-core Xtensa, 802.11 b/g/n
- **esp32c6**: RISC-V based, 802.11 ax (WiFi 6)

No target-specific code is used, maintaining the project's multi-target compatibility goals.

## Build & Test Instructions

### Full Build Process

```bash
# 1. Source ESP-IDF environment
source $HOME/esp/esp-idf/export.sh

# 2. Clean previous build (recommended)
idf.py fullclean

# 3. Configure (optional - iPerf is enabled by default)
idf.py menuconfig

# 4. Build
idf.py build

# 5. Flash and monitor
idf.py -p /dev/tty.usbmodem141201 flash monitor
```

### Testing the iPerf Server

```bash
# From a device connected to the ESP32 Access Point:

# 1. Verify connectivity
ping 192.168.4.1

# 2. Run TCP test (upload to ESP32)
iperf -c 192.168.4.1 -i 1 -t 10

# 3. Run UDP test
iperf -c 192.168.4.1 -u -b 20M -i 1 -t 10
```

### Expected Serial Output

```plaintext
I (12345) beaconbit: iPerf server started on port 5001
I (12346) beaconbit: Run 'iperf -c 192.168.4.1 -i 1 -t 30' from a connected client to test
```

## Troubleshooting

### Build Issues

**Problem:** `iperf.h: No such file or directory`

```bash
# Solution: Clean and rebuild to fetch components
idf.py fullclean
idf.py reconfigure
idf.py build
```

**Problem:** Linker errors about undefined iperf symbols

```bash
# Solution: Verify CMakeLists.txt has iperf in PRIV_REQUIRES
# Then clean and rebuild
idf.py fullclean
idf.py build
```

### Runtime Issues

**Problem:** iPerf server doesn't start

- Check: `CONFIG_BEACONBIT_IPERF_SERVER_ENABLE=y` in sdkconfig
- Check: Serial monitor for error messages
- Check: Sufficient heap memory available

**Problem:** Client can't connect to iPerf server

- Verify: Client is connected to ESP32 WiFi network
- Verify: Client can ping 192.168.4.1
- Verify: Port 5001 matches client's `-p` argument
- Check: Firewall on client isn't blocking outbound connections

## Future Enhancements

Potential improvements for future iterations:

- [ ] Web interface control (start/stop server via web UI)
- [ ] IPv6 support
- [ ] iPerf3 compatibility
- [ ] Real-time results display in web interface
- [ ] Automatic performance reports
- [ ] Historical performance tracking
- [ ] Configuration of interval and test duration via web UI

## References

- [ESP-IDF iPerf Component](https://components.espressif.com/components/espressif/iperf)
- [iPerf 2 Official Site](https://iperf.fr/)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)
