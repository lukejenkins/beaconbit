# 802.11 Country Code Support

## Overview

The BeaconBit project now supports configuring the 802.11 country code (regulatory domain) for WiFi operation. This feature ensures compliance with local regulations regarding WiFi channels and transmit power limits.

## What is a Country Code?

The country code is an ISO 3166-1 alpha-2 two-letter code (e.g., "US", "JP", "DE") that determines:

- **Available WiFi channels**: Different countries allow different channel ranges
- **Maximum transmit power**: Regulatory limits on RF power output
- **Channel spacing**: Some regions have different channel allocation rules

## Configuration

### Default Configuration (.env file)

Add the country code to your `.env` file:

```env
DEFAULT_COUNTRY_CODE="US"
```

Common country codes:

- `US` - United States (channels 1-11)
- `CA` - Canada (channels 1-11)
- `JP` - Japan (channels 1-14, channel 14 special case)
- `DE` - Germany (channels 1-13)
- `GB` - United Kingdom (channels 1-13)
- `FR` - France (channels 1-13)
- `AU` - Australia (channels 1-13)
- `CN` - China (channels 1-13)

For a complete list, see: <https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2>

### NVS Storage

The country code is stored in NVS as part of the configuration JSON:

```json
{
  "ssid": "ESP32-XXXX",
  "password": "...",
  "country_code": "US",
  "channel": 1,
  "bandwidth": 1,
  "max_connection": 4,
  "gtk_rekey_interval": 0,
  "auth_mode": "WPA2_PSK"
}
```

### Web Interface

The country code is displayed in the web configuration interface at `http://192.168.4.1/`.

### JSON API

The country code is included in the JSON API response at `/api/config`:

```json
{
  "ssid": "ESP32-XXXX",
  "has_password": true,
  "country_code": "US",
  "channel": 1,
  "bandwidth": 1,
  "max_connection": 4,
  "auth_mode": 3,
  "gtk_rekey_interval": 600
}
```

## Implementation Details

### Channel Availability

The firmware automatically adjusts the available channel count based on the country code:

- **US/CA**: Channels 1-11
- **JP**: Channels 1-14 (channel 14 is for 802.11b only)
- **Most of EU/Asia**: Channels 1-13

### Code Structure

The country code setting is applied in `main.c` using the `esp_wifi_set_country()` API:

```c
wifi_country_t country = {
    .cc = {cfg.country_code[0], cfg.country_code[1], 0},
    .schan = 1,          // Start channel
    .nchan = 13,         // Number of channels
    .policy = WIFI_COUNTRY_POLICY_AUTO
};

esp_wifi_set_country(&country);
```

## Best Practices

1. **Set the correct country code** for your deployment location to ensure regulatory compliance
2. **Verify channel availability** - some channels may not be available in all countries
3. **Consider transmit power** - some regions have stricter power limits
4. **Update after relocation** - if the device is moved to a different country, update the country code

## Regulatory Compliance

**Important**: Using the correct country code is essential for:

- **Legal compliance** with local telecommunications regulations
- **Avoiding interference** with licensed spectrum users
- **Ensuring device certification** (FCC, CE, etc.)

Always verify local regulations and ensure your WiFi configuration complies with applicable laws.

## Troubleshooting

### Country Code Not Applied

Check the serial console output for warnings:

```plaintext
W (1234) beaconbit: Failed to set country code US: ESP_ERR_WIFI_ARG
```

**Solution**: Ensure the country code is a valid 2-letter ISO 3166-1 alpha-2 code.

### Channels Not Available

Some channels may not be available in certain countries. The firmware logs the applied country settings:

```plaintext
I (1234) beaconbit: Country code set to: US (channels 1-11)
```

**Solution**: Select a channel within the available range for your country code.

### Default Fallback

If the country code is not specified in NVS, the firmware defaults to `"US"`.

## References

- [ISO 3166-1 alpha-2 Country Codes](https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2)
- [ESP-IDF WiFi API Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [WiFi Channel Allocations by Country](https://en.wikipedia.org/wiki/List_of_WLAN_channels)
