# BeaconBit - AI Agent Instructions

## Project Overview
This is an ESP-IDF project that creates a configurable Wi-Fi Access Point with a web interface for ESP32 microcontrollers. The project supports multiple ESP32 targets (esp32, esp32c3, esp32s3, esp32c6) and uses NVS (Non-Volatile Storage) for persistent configuration.

## Architecture & Key Components

### Core Components
- **`main/main.c`**: Entry point; initializes NVS, WiFi Access Point, and web server
- **`main/softap_config.{c,h}`**: NVS-backed configuration management with JSON serialization using cJSON
- **`main/softap_webserver.{c,h}`**: HTTP server providing HTML UI at `/` and JSON API at `/api/config`
- **`main/softap_env.h.in`**: CMake template for `.env`-based compile-time defaults

### Configuration System
Configuration is triple-layered:
1. **Build-time defaults** via `.env` file (optional) → processed by CMake into `softap_env.h`
2. **Kconfig options** in `main/Kconfig.projbuild` (legacy, overridden by NVS)
  3. **Runtime NVS storage** under namespace `beaconbit_cfg` as JSON blob (primary source of truth)

When NVS is empty on first boot, defaults are generated from `.env` + MAC address and saved to NVS.

## Critical Development Workflows

### Build & Flash (ESP-IDF Required)
```bash
# 1. Source ESP-IDF environment (adjust path to your IDF installation)
source $HOME/esp/esp-idf/export.sh

# 2. Set target chip (defaults to esp32c3 per sdkconfig)
export IDF_TARGET=esp32c3  # or esp32, esp32s3, esp32c6
idf.py set-target $IDF_TARGET

# 3. Configure via menuconfig (for WiFi features like WPA3)
idf.py menuconfig

# 4. Build, flash, and monitor
idf.py build
idf.py -p /dev/tty.usbmodem141201 flash monitor
```

### Configuration Management
```bash
# Reset config to defaults (erases all NVS)
idf.py erase-flash

# Create custom defaults before first boot
echo 'DEFAULT_SSID_PREFIX_TEMPLATE="MyAP-XXXX"' > .env
echo 'DEFAULT_AUTH_MODE="WPA2_PSK"' >> .env
# Then rebuild to regenerate softap_env.h
idf.py build
```

### Multi-Target Development
The project uses `idf_build_set_property(MINIMAL_BUILD ON)` in `CMakeLists.txt` to minimize component bloat. When targeting different chips:
1. **Always run `idf.py fullclean`** before switching targets
2. See `docs/MULTI_TARGET.md` for chip-to-target mappings
3. Run `tools/audit_target_api.py` to detect target-specific API usage (CI requirement)

## Project-Specific Conventions

### NVS Configuration Structure
```json
{
  "ssid": "BeaconBit-XXXX",
  "password": "...",
  "channel": 1,
  "bandwidth": 1,
  "max_connection": 4,
  "gtk_rekey_interval": 0,
  "auth_mode": "WPA2_PSK"  // or "WPA3_PSK"
}
```
- **Never modify NVS keys directly** — use `beaconbit_config_load()` / `beaconbit_config_save()`
- Password validation: empty for open networks, or 8-63 chars for WPA
- SSID template uses `XXXX` placeholder replaced with last 4 hex digits of MAC

### WiFi API Usage Pattern
**Critical**: Do NOT set `wifi_config.ap.bandwidth` directly in code (multi-target incompatibility). Always use:
```c
wifi_config_t wifi_config = {0};  // omit .ap.bandwidth field
ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, cfg.bandwidth));  // separate call
```
This pattern is enforced by `tools/audit_target_api.py` (fails CI if violated).

### WPA3 Support Gotcha
WPA3-PSK requires `CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT=y` in sdkconfig. To enable:
```bash
idf.py menuconfig
# Navigate: Component config → Wi-Fi → Enable SAE support
```
If WPA3 is configured in NVS but SAE is disabled, the code falls back to WPA2 automatically (see `main.c` lines 88-92).

## External Dependencies
- **ESP-IDF framework**: Must be sourced before any `idf.py` commands
- **cJSON**: Managed component from `managed_components/espressif__cJSON` (declared in `main/idf_component.yml`)
- **CMake-generated header**: `build/main/softap_env.h` is auto-generated from `.env` file at build time

## Data Flows
1. **Boot**: `app_main()` → NVS init → `beaconbit_config_load()` → generate defaults if missing → `wifi_init_softap()` → `esp_wifi_set_bandwidth()` + `esp_wifi_set_config()` → web server start
2. **Web request**: Client → ESP32 HTTP server → `root_get_handler()` or `api_config_get_handler()` → `beaconbit_config_load()` → render HTML/JSON
3. **Config update** (future feature): Web form POST → `beaconbit_config_save()` → `esp_restart()` to apply

## Testing & CI
- **Target API audit**: Run `python3 tools/audit_target_api.py` before commits (checks for target-specific code patterns)
- **Manual testing**: Connect to SSID shown in serial output, verify web UI at `http://192.168.4.1/`
- **Multi-target matrix**: GitHub Actions (planned) will build for esp32/esp32c3/esp32s3/esp32c6

## Key Files to Reference
- `README.md`: User-facing features and roadmap
- `docs/AUTH_MODE_CONFIG.md`: Authentication mode configuration details
- `docs/MULTI_TARGET.md`: Multi-chip build instructions
- `sdkconfig`: Current target config (esp32c3 by default), Kconfig-generated

## Common Pitfalls
1. **Forgetting to source ESP-IDF**: All `idf.py` commands require `. $IDF_PATH/export.sh` first
2. **Target mismatch**: Changing targets without `fullclean` causes linker errors
3. **Password in logs**: `main.c` logs password in plaintext (intentional for this use case — see line 118 comment)
4. **Bandwidth field access**: Using `.ap.bandwidth` directly instead of `esp_wifi_set_bandwidth()` breaks multi-target support

## Future Development Notes
- Web configuration form (POST handler) is planned but not implemented — see `TODO` in README.md
- Captive portal and client monitoring features are on roadmap
- Security (HTTPS) is explicitly NOT planned per README's "No plans to implement" section
