# Authentication Mode Configuration

## Overview

This softAP example now supports runtime configuration of the Wi-Fi authentication mode through NVS storage. Users can choose between WPA2-PSK and WPA3-PSK (WPA3-SAE) authentication modes.

## Configuration Methods

### 1. Default Configuration via .env File

Set the default authentication mode by creating a `.env` file in the project root:

```env
# Default authentication mode for softAP
# Options: WPA2_PSK, WPA3_PSK (also accepts WPA3-SAE)
DEFAULT_AUTH_MODE="WPA2_PSK"
```

This value is used when generating the default configuration on first boot.

### 2. Runtime Configuration via NVS

The authentication mode is stored in NVS as part of the softAP configuration JSON object. The configuration is saved in the `softap_cfg` namespace under the key `json`.

Example JSON structure:

```json
{
  "ssid": "ESP32-A1B2",
  "password": "mypassword",
  "channel": 1,
  "bandwidth": 1,
  "max_connection": 4,
  "gtk_rekey_interval": 0,
  "auth_mode": "WPA3_PSK"
}
```

## Supported Authentication Modes

- `WPA2_PSK`: WPA2-Personal (default, most compatible)
- `WPA3_PSK` or `WPA3-SAE`: WPA3-Personal with SAE (more secure, requires device support)

## Requirements for WPA3

To use WPA3-PSK authentication, you must:

1. Enable SAE support in `sdkconfig`:

   ```bash
   idf.py menuconfig
   ```

   Navigate to: `Component config` → `Wi-Fi` → Enable `Enable SAE support`

2. Your ESP32 chip and SDK version must support WPA3

3. Client devices must support WPA3-SAE

## Behavior

- If WPA3 is configured but not supported (SAE not enabled), the system will:
  - Log a warning message
  - Fall back to WPA2-PSK automatically
  
- If password is empty, authentication mode is ignored and the AP runs as OPEN

- PMF (Protected Management Frames) is required for WPA3 and automatically configured

## Modifying Configuration

### Reset to Defaults

To regenerate the default configuration:

```bash
idf.py erase-flash
idf.py flash monitor
```

### Programmatic Changes

Modify the configuration by calling:

```c
#include "softap_config.h"

softap_config_t cfg;
softap_config_load(&cfg);

// Change auth mode
cfg.auth_mode = WIFI_AUTH_WPA3_PSK;

// Save changes
softap_config_save(&cfg);

// Restart to apply
esp_restart();
```

## Security Considerations

- **WPA3-PSK** provides better security through SAE (Simultaneous Authentication of Equals)
- **WPA2-PSK** is more widely compatible but less secure
- Always use a strong password (8-63 characters)
- Consider enabling GTK rekeying for additional security

## Example Output

When WPA3 is configured and supported:

```bash
I (123) wifi softAP: wifi_init_softap finished. SSID:ESP32-A1B2 password:mypassword channel:1 auth:WPA3-PSK
```

When WPA2 is configured:

```bash
I (123) wifi softAP: wifi_init_softap finished. SSID:ESP32-A1B2 password:mypassword channel:1 auth:WPA2-PSK
```
