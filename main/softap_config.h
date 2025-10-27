#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi.h"

#define SOFTAP_CONFIG_NAMESPACE "softap_cfg"

typedef struct {
    char ssid[33];
    char password[65];
    char country_code[3]; // ISO 3166-1 alpha-2 country code (e.g., "US", "JP", "DE")
    char wps_device_name[33]; // WPS device name (max 32 chars + null terminator)
    int channel;
    int max_connection;
    int gtk_rekey_interval;
    int bandwidth; // use WIFI_BW_* values (stored as int)
    bool auth_open;
    wifi_auth_mode_t auth_mode; // WIFI_AUTH_WPA2_PSK or WIFI_AUTH_WPA3_PSK
} softap_config_t;

// Load config from NVS. Returns ESP_OK if loaded, ESP_ERR_NVS_NOT_FOUND if not present, or other errors.
esp_err_t softap_config_load(softap_config_t *out_config);

// Save config to NVS (serializes to JSON internally).
esp_err_t softap_config_save(const softap_config_t *config);

// Generate default config according to generation rules. Caller should then save if desired.
void softap_config_generate_default(softap_config_t *out_config);
