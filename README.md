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
DEFAULT_AUTH_MODE="WPA3_PSK"
```

**Note**: WPA3 requires `CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT` to be enabled in `sdkconfig`. Enable this via `idf.py menuconfig` under `Component config` → `Wi-Fi` → `Enable SAE support`.

#### Runtime Configuration via NVS

The configuration is stored in NVS under the namespace `softap_cfg` as a JSON object. You can modify it programmatically or by erasing NVS to regenerate defaults:

```bash
# Erase NVS to regenerate default configuration
idf.py erase-flash
```

## Issues

- [ ] ESP32 seems to be stuck on wpa3

## To-Do

- [ ] Add a web server to serve a configuration page for runtime configuration changes.
- [ ] Add a captive portal to redirect clients to the configuration page upon connection.
- [ ] Add a command-line interface (CLI) for configuration via serial console.
- [ ] Add RSSI display for connected clients via web server.
- [ ] Add RSSI display for connected clients via CLI.
- [ ] Add a mechanism to save configuration changes persistently.
- [ ] Add a mechanism to reset configuration to defaults.
- [ ] Add support for IPv6.
- [ ] Add support for event logging (e.g., client connect/disconnect events).
- [ ] Add support for monitoring and displaying connected clients (e.g., via web server or CLI).
- [ ] Add support for multiple SSIDs (if hardware supports it).
