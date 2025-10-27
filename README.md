# Expanded Wi-Fi SoftAP

This is an expansion of the abilities of the original Wi-Fi SoftAP example provided in ESP-IDF. It includes additional configuration options.

## How to use example

SoftAP supports Protected Management Frames(PMF). Necessary configurations can be set using pmf flags. Please refer [Wifi-Security](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi-security.html) for more info.

### Configuration

This example uses NVS (Non-Volatile Storage) to persist Wi-Fi configuration. On first boot, it generates a default configuration and saves it to NVS. The configuration includes:

- **SSID**: Generated from template with MAC address suffix
- **Password**: Auto-generated or empty (for open networks)
- **Channel**: Default channel 1
- **Authentication Mode**: WPA2-PSK or WPA3-PSK (WPA3-SAE)
- **Bandwidth**: 20MHz or 40MHz
- **Max Connections**: Maximum number of stations allowed
- **GTK Rekey Interval**: Group Temporal Key rekeying interval

#### Environment Variables (.env file)

You can customize default values by creating a `.env` file in the project root. See `.env.example` for available options:

- `DEFAULT_SSID_PREFIX_TEMPLATE`: SSID template (use 'XXXX' for MAC address suffix)
- `DEFAULT_AUTH_MODE`: Default authentication mode (`WPA2_PSK` or `WPA3_PSK`)

Example `.env` file:

```env
DEFAULT_SSID_PREFIX_TEMPLATE="MyAP-XXXX"
DEFAULT_AUTH_MODE="WPA2_PSK"  # or "WPA3_PSK"
```

**Authentication Mode Behavior**:

- The authentication mode is **runtime-configurable** via NVS and is independent of the SDK configuration
- If you request WPA3 (either via `.env` default or NVS configuration) but `CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT` is not enabled in `sdkconfig`, the firmware will automatically fall back to WPA2-PSK with a warning in the logs
- To enable WPA3 support, use `idf.py menuconfig` → `Component config` → `Wi-Fi` → `Enable SAE support`
- The actual authentication mode in use is displayed in the startup logs (e.g., `auth:WPA2-PSK` or `auth:WPA3-PSK`)

#### Runtime Configuration via NVS

The configuration is stored in NVS under the namespace `softap_cfg` as a JSON object. You can modify it programmatically or by erasing NVS to regenerate defaults:

```bash
# Erase NVS to regenerate default configuration
idf.py erase-flash
```

### Web Server Configuration Interface

The project includes a built-in web server that provides a user-friendly interface to view the current SoftAP configuration.

#### Accessing the Web Interface

1. **Connect to the ESP32's WiFi network** using the SSID shown in the serial output
2. **Open a web browser** and navigate to: `http://192.168.4.1/`
3. **View the configuration** displayed in a responsive, styled table

#### Features

- **Web UI** (`/`): Displays all current configuration settings including:
  - SSID and authentication mode
  - Channel and bandwidth
  - Maximum connections
  - GTK rekey interval
  - Password (masked for security)

- **JSON API** (`/api/config`): Returns configuration as JSON for programmatic access

  ```json
  {
    "ssid": "ESP32-XXXX",
    "has_password": true,
    "channel": 1,
    "bandwidth": 1,
    "max_connection": 4,
    "auth_mode": 3,
    "gtk_rekey_interval": 600
  }
  ```

The web interface is responsive and works well on both desktop and mobile devices.

## Issues (Resolved)

- [X] **ESP32 authentication mode configuration decoupled from sdkconfig** (Fixed: Oct 2025)
  - **Problem**: Auth mode was determined solely by `CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT` at compile time, ignoring the NVS configuration
  - **Solution**: Auth mode is now runtime-configurable via NVS as the primary source of truth
  - **Behavior**:
    - Set `DEFAULT_AUTH_MODE="WPA2_PSK"` in `.env` for WPA2 by default
    - Set `DEFAULT_AUTH_MODE="WPA3_PSK"` in `.env` for WPA3 by default (with automatic fallback to WPA2 if SAE support isn't compiled in)
    - Change auth mode at runtime by modifying the NVS configuration
    - The firmware will gracefully fall back to WPA2 if WPA3 is requested but SAE support is not available, logging a clear warning

## Completed Features

- [X] Web server for viewing configuration
- [X] JSON API endpoint for programmatic access
- [X] NVS-based persistent configuration storage
- [X] WPA2/WPA3 authentication support
- [X] Protected Management Frames (PMF)
- [X] GTK rekeying support
- [X] Environment variable-based default configuration
- [X] MAC address-based SSID generation

## To-Do / Roadmap

### High Priority

- [ ] Add channel selection to the web interface
  - Allow users to select the Wi-Fi channel for the SoftAP
  - Update NVS configuration accordingly
  - If channel is not already set in NVS, set channel to 1, 6, or 11 based on mac address in some way so that each unit has ~33% chance of being on one of those channels
- [ ] Add additional configurable options to the NVS config.
- [ ] Add web form for editing configuration via web interface
  - Input fields for everything stored in NVS.
  - Save to NVS functionality
  - Apply changes without restart (if possible)
- [ ] Add captive portal to redirect clients to the configuration page upon connection
- [ ] Add "Reset to Defaults" button in web interface

### Medium Priority

- [ ] Add monitoring and display of connected clients
  - Show MAC addresses, connection time, IP addresses
  - Display via web interface
- [ ] Add RSSI (signal strength) display for connected clients
- [ ] Add event logging system
  - Client connect/disconnect events with timestamps
  - Configuration change history
  - Viewable via web interface
- [ ] Speed testing services to measure throughput of the connected clients
  - [ ] iperf server
  - [ ] Web interface for speed tests

### Low Priority / Future Enhancements

- [ ] Add command-line interface (CLI) for configuration via serial console
- [ ] Add support for IPv6
- [ ] Add support for multiple SSIDs (if hardware supports it)
- [ ] Add mDNS support for easy discovery (e.g., `http://esp32-config.local`)
- [ ] Add WPS Device Name support in SoftAP settings
- [X] Add the ability to configure WPA2/WPA3 settings independently of sdkconfig

### No plans to implement

- ~~[ ]~~ Security enhancements (e.g., HTTPS for web server, password for web interface).
  - The current use case for this device does not necessitate security for the web interface.

## AI Disclosure

**Here there be robots!** I *think* they are friendly, but they might just be very good at pretending. You might be a fool if you use this project for anything other than as an example of how silly it can be to use AI to code with.

> This project was developed with the assistance of language models from OpenAI and Anthropic, which provided suggestions and code snippets to enhance the functionality and efficiency of the tools. The models were used to generate code, documentation, distraction, moral support, moral turpitude, and explanations for various components of the project.
