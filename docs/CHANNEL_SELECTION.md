# WiFi Channel Selection Feature

## Overview

This document describes the WiFi channel selection feature implementation for the ESP32 SoftAP project.

## Features Implemented

### 1. MAC-Based Default Channel Assignment

When a device boots for the first time (no NVS configuration exists), it automatically selects a WiFi channel based on its MAC address. This ensures an even distribution of devices across the three non-overlapping 2.4GHz WiFi channels (1, 6, and 11).

**Algorithm:**
- Uses the last byte of the MAC address modulo 3
- Maps results to channels: 0 → Channel 1, 1 → Channel 6, 2 → Channel 11
- Simple and effective: provides ~33% distribution per channel

**Implementation Location:**
- `main/softap_config.c` - `softap_config_generate_default()` function

**Testing:**
A test program was created to validate the distribution algorithm:
- Location: `tools/test_channel_selection.c`
- Tested against 21 real MAC addresses (17 unique)
- Results: 29.4% on Channel 1, 29.4% on Channel 6, 41.2% on Channel 11
- Distribution is reasonably balanced across the three channels

To run the test:
```bash
cd tools
gcc -o test_channel_selection test_channel_selection.c
./test_channel_selection
```

### 2. Web Interface for Channel Selection

Users can now change the WiFi channel through the web interface without reflashing firmware.

**Features:**
- Dropdown menu with all 2.4GHz channels (1-13)
- Channels 1, 6, and 11 marked as "Recommended" (non-overlapping)
- Frequency display for each channel
- Real-time feedback on configuration update
- Automatic page reload after successful update

**Implementation Details:**
- HTML form with channel dropdown
- JavaScript handles form submission via fetch API
- POST request to `/api/config` endpoint
- Configuration saved to NVS
- Device requires restart for changes to take effect

### 3. API Endpoint for Configuration Updates

**POST /api/config**
- Accepts JSON payload with configuration parameters
- Currently supports channel updates
- Validates channel range (1-13)
- Saves to NVS and returns success/error message

**Example Request:**
```bash
curl -X POST http://192.168.4.1/api/config \
  -H "Content-Type: application/json" \
  -d '{"channel": 6}'
```

**Example Response:**
```json
{
  "success": true,
  "message": "Configuration updated. Restart required for changes to take effect."
}
```

## Why Channels 1, 6, and 11?

In the 2.4GHz WiFi band, channels overlap with adjacent channels. Channels 1, 6, and 11 are the only three channels that don't overlap with each other, making them ideal for minimizing interference in environments with multiple access points.

**Channel Spacing:**
- Each channel is 5 MHz apart in frequency
- Each channel uses 22 MHz bandwidth
- Channels 1, 6, and 11 are separated by 25 MHz, preventing overlap

## Configuration Storage

Channel configuration is stored in NVS (Non-Volatile Storage) under the namespace `softap_cfg` as part of the JSON configuration object:

```json
{
  "ssid": "ESP32-XXXX",
  "password": "...",
  "channel": 6,
  "bandwidth": 1,
  "max_connection": 4,
  "gtk_rekey_interval": 0,
  "auth_mode": "WPA2_PSK"
}
```

## User Experience Flow

1. **First Boot:** Device auto-selects channel based on MAC address (1, 6, or 11)
2. **Access Web Interface:** User connects to SoftAP and navigates to `http://192.168.4.1/`
3. **View Current Channel:** Configuration table shows current channel
4. **Change Channel:** User selects new channel from dropdown and clicks "Update Channel"
5. **Confirmation:** Success message displayed, page automatically reloads
6. **Apply Changes:** User restarts ESP32 for new channel to take effect

## Technical Implementation Details

### Files Modified

1. **main/softap_config.c**
   - Modified `softap_config_generate_default()` to implement MAC-based channel selection
   - Algorithm uses last byte of MAC address modulo 3

2. **main/softap_webserver.c**
   - Added CSS styles for form elements and messages
   - Enhanced `root_get_handler()` to include channel selection form with JavaScript
   - Added `api_config_post_handler()` to handle configuration updates
   - Registered POST handler for `/api/config` endpoint

3. **tools/test_channel_selection.c** (new)
   - Test program to validate channel distribution algorithm
   - Parses MAC addresses and tests distribution across channels

4. **README.md**
   - Updated completed features section
   - Removed channel selection from to-do list

5. **docs/CHANNEL_SELECTION.md** (this document)
   - Comprehensive documentation of the feature

### Code Quality

- All code follows ESP-IDF conventions
- Proper error handling for API requests
- Input validation for channel range (1-13)
- Clear user feedback for success/error states
- Responsive design works on mobile and desktop

## Future Enhancements

Potential improvements for this feature:

1. **Auto-restart option:** Add checkbox to automatically restart after configuration update
2. **Channel scan:** Display current channel usage before selection
3. **Signal strength monitoring:** Show RSSI for connected clients
4. **Bandwidth selection:** Allow users to change between 20MHz and 40MHz
5. **More configuration options:** Extend POST API to handle all configuration parameters

## References

- [ESP-IDF WiFi API Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [WiFi 2.4GHz Channel Information](https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax))
- ESP32 SoftAP Configuration: `main/softap_config.h`
- Web Server Implementation: `main/softap_webserver.h`
