# WPS Information Elements in Beacon Frames

## Overview

This document explains how WPS (Wi-Fi Protected Setup) Information Elements are included in 802.11 beacon frames and probe responses, and the custom vendor-specific IE implementation that supplements the standard WPS IEs.

## Standard WPS Behavior

Per the Wi-Fi Alliance WPS specification and ESP-IDF implementation:

### Beacon Frames (Minimal IE - 24 bytes)

Beacon frames contain a **minimal WPS Information Element** with only essential fields:

- **Version**: WPS version (0x10 = v1.0, 0x20 = v2.0)
- **WiFi Protected Setup State**: Configured (0x02) or Not Configured (0x01)
- **Vendor Extension**: Version2 information

**Example from packet capture:**

```plaintext
Tag: Vendor Specific: Microsoft Corp.: WPS
    Tag Number: Vendor Specific (221)
    Tag length: 24
    OUI: 00:50:f2 (Microsoft Corp.)
    Vendor Specific OUI Type: 4
    Version: 0x10
    Wifi Protected Setup State: Configured (0x02)
    Vendor Extension: 00372a000120
```

### Probe Responses (Full IE - 144+ bytes)

Probe responses contain the **complete WPS Information Element** with all device information:

- Version
- WiFi Protected Setup State
- Response Type
- UUID E (Universally Unique Identifier for Enrollee)
- **Manufacturer**: "ESPRESSIF"
- **Model Name**: "ESP SoftAP"
- **Model Number**: Target chip (e.g., "esp32c3")
- **Serial Number**: MAC address-based
- Primary Device Type
- **Device Name**: Configured device name (e.g., "ESP32-07ED")
- Config Methods

**Why the difference?**

- Beacons are sent **every 100ms** (10 times/second) to all devices in range
- Keeping beacons small (24 bytes) reduces airtime and network overhead
- Probe responses are only sent when a client actively scans, so full details can be included

## Custom Vendor-Specific IE for Device Name in Beacons

Since the standard WPS implementation in ESP-IDF doesn't include the device name in beacon frames, this project adds a **vendor-specific Information Element** to beacon frames using the **Alcatel-Lucent OUI** (0xDC:08:56).

### Why Alcatel-Lucent OUI?

The Alcatel-Lucent OUI is already recognized by Wireshark and other protocol analyzers as `wlan.vs.alcatel.apname`, which means:

- **No custom dissector needed**: The AP Name is automatically decoded and displayed
- **Immediate visibility**: Wireshark shows the device name in the packet details
- **Industry standard**: This OUI/type combination is already used by Alcatel-Lucent access points
- **No registration required**: We're using an existing, recognized vendor IE format

**Note**: This is technically "borrowing" Alcatel-Lucent's OUI. While this works perfectly for development, testing, and educational purposes, production deployments in commercial environments should ideally use a properly registered OUI to avoid potential conflicts.

### Implementation

The Alcatel-Lucent AP Name IE is added using `esp_wifi_set_vendor_ie()` after starting WiFi:

```c
// Structure: vendor_ie_data_t header + device name payload (ASCII)
vendor_ie_data_t *vie = (vendor_ie_data_t *)device_name_ie;
vie->element_id = WIFI_VENDOR_IE_ELEMENT_ID; // 0xDD (221)
vie->length = 4 + device_name_len; // OUI (3) + Type (1) + payload
vie->vendor_oui[0] = 0xDC; // Alcatel-Lucent OUI
vie->vendor_oui[1] = 0x08;
vie->vendor_oui[2] = 0x56;
vie->vendor_oui_type = 0x01; // Type: AP Name (ALCATEL_APNAME)
memcpy(vie->payload, cfg.wps_device_name, device_name_len);

esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_1, vie);
```

### Packet Capture Format

In Wireshark or other packet capture tools, you will see **two vendor-specific IEs** in beacon frames:

1. **Standard WPS IE** (OUI: 00:50:f2, Type: 4) - 24 bytes
   - Contains version, state, vendor extension
2. **Alcatel-Lucent AP Name IE** (OUI: DC:08:56, Type: 1) - Variable length
   - Wireshark automatically decodes this as `wlan.vs.alcatel.apname`
   - Contains the configured device name as ASCII string

**Example Wireshark Display:**

```plaintext
Tag: Vendor Specific: Microsoft Corp.: WPS
    OUI: 00:50:f2
    Type: 4 (WPS)
    Length: 24 bytes
    [Standard WPS minimal content]

Tag: Vendor Specific: Alcatel-Lucent: AP Name
    OUI: DC:08:56 (Alcatel-Lucent)
    Type: 1 (AP Name)
    Length: 25 bytes (6 byte header + 19 byte AP name)
    AP Name: ESP32-07ED
```

**Wireshark Filter:**

```plaintext
wlan.vs.alcatel.apname
```

This filter will show all frames containing the Alcatel-Lucent AP Name IE.

## OUI Usage: Alcatel-Lucent vs. Custom Registration

This implementation uses the **Alcatel-Lucent OUI (0xDC:08:56)** with their "AP Name" vendor-specific IE type. This approach has both benefits and considerations:

### Benefits of Using Alcatel-Lucent OUI

**Immediate Wireshark Support:**

- Wireshark automatically recognizes `OUI: DC:08:56, Type: 0x01` as "AP Name"
- The device name is decoded and displayed as `wlan.vs.alcatel.apname`
- No custom dissector or configuration needed
- Works out-of-the-box with standard Wireshark installations

**Filtering & Analysis:**

- Easy filtering: `wlan.vs.alcatel.apname`
- Can extract AP names from large captures
- Compatible with existing network analysis tools

### Use Case Considerations

**Appropriate for:**

- Development and testing purposes
- Educational environments and classroom use
- Closed/private networks
- Proof-of-concept deployments
- Research and experimentation

**Considerations for Production:**

While using another vendor's OUI is not technically illegal and is common practice in development/testing, production deployments should consider:

1. **Potential conflicts**: If Alcatel-Lucent devices exist on the same network, there could be confusion
2. **Vendor identification**: Network tools will identify your ESP32 devices as "Alcatel-Lucent"
3. **Professional appearance**: Enterprise deployments may prefer registered OUIs

### For Production Use

If deploying this system in production or public networks, you should:

1. **Register an OUI** with IEEE (<https://standards.ieee.org/products-programs/regauth/>)
   - **OUI-24**: $3,190 for 16.7 million addresses (e.g., AA:BB:CC:xx:xx:xx)
   - **MA-L**: Same as OUI-24, preferred term
   - **MA-M**: $1,775 for 1 million addresses
   - **MA-S**: $570 for 4,096 addresses
2. **Use a Company Identifier** (CID) if you're a Wi-Fi Alliance member
3. **Or use an existing registered OUI** if your organization already has one

### Updating the OUI (If Registering Your Own)

To change to a registered OUI, modify `main/main.c`:

```c
vie->vendor_oui[0] = 0xYY; // Your registered OUI byte 1
vie->vendor_oui[1] = 0xYY; // Your registered OUI byte 2
vie->vendor_oui[2] = 0xYY; // Your registered OUI byte 3
vie->vendor_oui_type = 0x01; // Your custom type identifier
```

Note: If you use a custom OUI, you'll need to create a Wireshark dissector for automatic decoding.

## Broadcast Frequency

Both IEs are broadcast:

- **In beacon frames**: Every 100ms (10 times/second)
- **In probe responses**: When a client performs active scanning
- **Continuous**: As long as the SoftAP is active

## Benefits of Custom IE in Beacons

1. **Immediate visibility**: Clients can see device name without sending probe requests
2. **Reduced latency**: No need for probe request/response exchange
3. **Compatible with passive scanning**: Clients using passive scanning (beacons only) can see device name
4. **Small overhead**: ~25 bytes per beacon for typical device names

## Limitations

1. **No standards compliance**: Custom IE is not part of WPS specification
2. **Limited client support**: Most WiFi scanning apps won't decode the custom IE by default
3. **Increased beacon size**: Adds ~25 bytes to each beacon frame
4. **OUI registration**: Should register OUI for production use

## Verification

### Using Wireshark

1. Start packet capture on WiFi channel
2. Filter: `wlan.addr == [ESP32_MAC_ADDRESS] && wlan.fc.type_subtype == 0x08` (beacons)
3. Look for two Vendor Specific IEs:
   - First: OUI 00:50:f2 (Microsoft/WPS)
   - Second: OUI AA:BB:CC (Custom device name)

### Using Command Line (macOS/Linux)

```bash
# Capture beacon frames from ESP32 AP
sudo tcpdump -i en0 -e -vvv -s 0 type mgt subtype beacon | grep -A 10 "ESP32"
```

### Using ESP-IDF Logging

The firmware logs when the custom IE is successfully added:

```plaintext
I (123) wifi softAP: Custom device name IE added to beacons: ESP32-07ED
```

Or if it fails:

```plaintext
W (123) wifi softAP: Failed to add custom device name IE to beacons: [error]
```

## Alternative Approaches Not Implemented

### Option 1: Modify wpa_supplicant Source

Modify ESP-IDF's wpa_supplicant component to include device name in the standard WPS beacon IE. This would:

- ✅ Be WPS specification compliant (device name is an optional field)
- ✅ Work with all WPS-aware clients
- ❌ Require maintaining a fork of wpa_supplicant
- ❌ Require rebuilding wpa_supplicant component
- ❌ May break WPS certification
- ❌ More complex to maintain across ESP-IDF versions

### Option 2: Use SSID for Device Name

Simply set the SSID to the desired device name. This:

- ✅ Is universally supported
- ✅ Visible to all WiFi clients
- ✅ No custom implementation needed
- ❌ Doesn't follow WPS device name convention
- ❌ Loses the separate identity concept (network name vs device name)
- ❌ May have SSID length limitations (32 bytes)

### Option 3: DNS-SD / mDNS Announcement

Use mDNS to announce device name on the network. This:

- ✅ Is a standard protocol (Bonjour/Zeroconf)
- ✅ Works well for service discovery
- ❌ Requires clients to be **connected** to see the device name
- ❌ Not visible during WiFi scanning phase
- ❌ Adds network traffic overhead

## Benefits of Alcatel-Lucent OUI Approach

The Alcatel-Lucent vendor IE approach provides significant advantages over custom implementations:

1. **Immediate visibility**: Device name visible during scanning, before connection
2. **Zero configuration**: Works out-of-the-box with Wireshark and other tools
3. **Simple implementation**: ~35 lines of code, no external dependencies
4. **Non-invasive**: Doesn't modify ESP-IDF components
5. **Maintains WPS compatibility**: Standard WPS still works normally
6. **Low overhead**: Small addition (~25 bytes) to beacon frames
7. **Industry standard format**: Uses established vendor IE pattern
8. **Easy debugging**: Wireshark filter `wlan.vs.alcatel.apname` shows device names instantly

## Configuration

The WPS and AP Name IE features can be independently enabled or disabled.

### Configuration Options

Both features are controlled through NVS configuration and can have defaults set via `.env` file:

1. **`wps_enabled`** (boolean) - Enable/disable WPS (Wi-Fi Protected Setup)
   - When enabled: WPS IEs are included in probe responses with full device information
   - When disabled: No WPS functionality, probe responses don't include WPS IEs
   - Default: `true` (enabled)

2. **`apname_ie_enabled`** (boolean) - Enable/disable Alcatel-Lucent AP Name IE in beacons
   - When enabled: Device name is broadcast in beacon frames every 100ms
   - When disabled: Device name only appears in probe responses (if WPS enabled)
   - Default: `true` (enabled)

3. **`wps_device_name`** (string, max 32 chars) - The device name to broadcast
   - Used by both WPS (if enabled) and AP Name IE (if enabled)
   - Defaults to SSID if not specified

### .env File Configuration

```env
# Enable/disable WPS in SoftAP mode
DEFAULT_WPS_ENABLED="true"

# Enable/disable Alcatel-Lucent AP Name IE in beacons
DEFAULT_APNAME_IE_ENABLED="true"

# Device name template (use 'XXXX' for MAC address suffix)
DEFAULT_WPS_DEVICE_NAME="MyDevice-XXXX"
```

### NVS Configuration (JSON)

```json
{
  "ssid": "MyAP-1234",
  "password": "mypassword",
  "wps_device_name": "MyDevice-1234",
  "wps_enabled": true,
  "apname_ie_enabled": true,
  "channel": 1,
  "bandwidth": 1,
  "max_connection": 4,
  "auth_mode": "WPA2_PSK"
}
```

### Web Interface

Both settings are displayed in the web configuration interface at `http://192.168.4.1/`:

- **WPS Enabled**: Yes/No
- **AP Name IE Enabled**: Yes/No
- **WPS Device Name**: Current device name

### JSON API

The settings are included in the JSON API response at `/api/config`:

```json
{
  "ssid": "MyAP-1234",
  "wps_device_name": "MyDevice-1234",
  "wps_enabled": true,
  "apname_ie_enabled": true,
  "channel": 1,
  ...
}
```

### Configuration Combinations

| WPS Enabled | AP Name IE Enabled | Behavior |
|-------------|-------------------|----------|
| ✅ Yes | ✅ Yes | Full device name in both beacons (Alcatel IE) and probe responses (WPS IE) |
| ✅ Yes | ❌ No | Device name only in probe responses (WPS IE), minimal WPS IE in beacons |
| ❌ No | ✅ Yes | Device name in beacons (Alcatel IE), no WPS functionality |
| ❌ No | ❌ No | No device name broadcast, no WPS functionality |

### Runtime Changes

To change these settings at runtime:

```c
#include "softap_config.h"

softap_config_t cfg;
softap_config_load(&cfg);

// Enable/disable WPS
cfg.wps_enabled = true;  // or false

// Enable/disable AP Name IE
cfg.apname_ie_enabled = true;  // or false

// Change device name
strncpy(cfg.wps_device_name, "NewName-1234", sizeof(cfg.wps_device_name) - 1);

// Save changes
softap_config_save(&cfg);

// Restart to apply
esp_restart();
```

## Future Enhancements

- [ ] Make OUI configurable via `.env` or NVS
- [ ] Add configuration option to disable custom IE (use only standard WPS)
- [ ] Support multiple custom IEs (manufacturer, model, etc.)
- [ ] Add web UI for changing device name at runtime
- [ ] Implement IE verification/validation on boot

## References

- [Wi-Fi Alliance WPS Specification](https://www.wi-fi.org/discover-wi-fi/wi-fi-protected-setup)
- [IEEE 802.11 Standard](https://standards.ieee.org/standard/802_11-2020.html)
- [ESP-IDF WiFi API Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [IEEE Registration Authority](https://standards.ieee.org/products-programs/regauth/)
- [ESP-IDF Vendor IE Example](https://github.com/espressif/esp-idf/tree/master/examples/wifi/vendor_ie)
