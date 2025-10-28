#include "sdkconfig.h"
#include "softap_config.h"
#include <string.h>
#include <stdlib.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "cJSON.h"
#include "softap_env.h"

static const char *TAG = "softap_config";

esp_err_t softap_config_load(softap_config_t *out_config)
{
    if (!out_config) return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t err = nvs_open(SOFTAP_CONFIG_NAMESPACE, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_NVS_NOT_INITIALIZED) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (err != ESP_OK) return err;

    size_t required_size = 0;
    err = nvs_get_str(h, "json", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(h);
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    }

    char *buf = malloc(required_size);
    if (!buf) {
        nvs_close(h);
        return ESP_ERR_NO_MEM;
    }
    err = nvs_get_str(h, "json", buf, &required_size);
    nvs_close(h);
    if (err != ESP_OK) {
        free(buf);
        return err;
    }

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        ESP_LOGW(TAG, "Failed to parse JSON config");
        return ESP_FAIL;
    }

    const cJSON *jssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    const cJSON *jpass = cJSON_GetObjectItemCaseSensitive(root, "password");
    const cJSON *jcountry = cJSON_GetObjectItemCaseSensitive(root, "country_code");
    const cJSON *jwps = cJSON_GetObjectItemCaseSensitive(root, "wps_device_name");
    const cJSON *jwps_enabled = cJSON_GetObjectItemCaseSensitive(root, "wps_enabled");
    const cJSON *japname_ie = cJSON_GetObjectItemCaseSensitive(root, "apname_ie_enabled");
    const cJSON *jchannel = cJSON_GetObjectItemCaseSensitive(root, "channel");
    const cJSON *jmaxconn = cJSON_GetObjectItemCaseSensitive(root, "max_connection");
    const cJSON *jband = cJSON_GetObjectItemCaseSensitive(root, "bandwidth");
    const cJSON *jgtk = cJSON_GetObjectItemCaseSensitive(root, "gtk_rekey_interval");
    const cJSON *jauth = cJSON_GetObjectItemCaseSensitive(root, "auth_mode");

    memset(out_config, 0, sizeof(*out_config));
    if (cJSON_IsString(jssid) && (jssid->valuestring != NULL)) {
        strncpy(out_config->ssid, jssid->valuestring, sizeof(out_config->ssid)-1);
    }
    if (cJSON_IsString(jpass) && (jpass->valuestring != NULL)) {
        strncpy(out_config->password, jpass->valuestring, sizeof(out_config->password)-1);
    }
    if (cJSON_IsString(jcountry) && (jcountry->valuestring != NULL)) {
        strncpy(out_config->country_code, jcountry->valuestring, sizeof(out_config->country_code)-1);
        out_config->country_code[sizeof(out_config->country_code)-1] = '\0';
    } else {
        // Default to "US" if not specified
        memcpy(out_config->country_code, "US", 2);
        out_config->country_code[2] = '\0';
    }
    if (cJSON_IsString(jwps) && (jwps->valuestring != NULL)) {
        strncpy(out_config->wps_device_name, jwps->valuestring, sizeof(out_config->wps_device_name)-1);
    } else {
        // Default to SSID if not specified
        strncpy(out_config->wps_device_name, out_config->ssid, sizeof(out_config->wps_device_name)-1);
    }
    if (cJSON_IsBool(jwps_enabled)) {
        out_config->wps_enabled = cJSON_IsTrue(jwps_enabled);
    } else {
        out_config->wps_enabled = true; // default: enabled
    }
    if (cJSON_IsBool(japname_ie)) {
        out_config->apname_ie_enabled = cJSON_IsTrue(japname_ie);
    } else {
        out_config->apname_ie_enabled = true; // default: enabled
    }
    if (cJSON_IsNumber(jchannel)) {
        out_config->channel = jchannel->valueint;
    } else {
        out_config->channel = 1;
    }
    if (cJSON_IsNumber(jband)) {
        out_config->bandwidth = jband->valueint;
    } else {
        out_config->bandwidth = WIFI_BW_HT20;
    }
    if (cJSON_IsNumber(jmaxconn)) {
        out_config->max_connection = jmaxconn->valueint;
    } else {
        out_config->max_connection = 4;
    }
    if (cJSON_IsNumber(jgtk)) {
        out_config->gtk_rekey_interval = jgtk->valueint;
    } else {
        out_config->gtk_rekey_interval = 0;
    }
    if (cJSON_IsString(jauth) && (jauth->valuestring != NULL)) {
        // Parse auth mode string
        if (strcmp(jauth->valuestring, "WPA3_PSK") == 0 || strcmp(jauth->valuestring, "WPA3-SAE") == 0) {
            out_config->auth_mode = WIFI_AUTH_WPA3_PSK;
        } else if (strcmp(jauth->valuestring, "WPA2_PSK") == 0) {
            out_config->auth_mode = WIFI_AUTH_WPA2_PSK;
        } else {
            out_config->auth_mode = WIFI_AUTH_WPA2_PSK; // default
        }
    } else {
        out_config->auth_mode = WIFI_AUTH_WPA2_PSK; // default
    }

    out_config->auth_open = (strlen(out_config->password) == 0);

    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t softap_config_save(const softap_config_t *config)
{
    if (!config) return ESP_ERR_INVALID_ARG;

    // Validate SSID length (must be 1-32 bytes)
    if (strlen(config->ssid) == 0 || strlen(config->ssid) > 32) {
        return ESP_ERR_INVALID_ARG;
    }
    // Validate password (0 for open, or 8-63 for WPA)
    size_t pass_len = strlen(config->password);
    if (pass_len != 0 && (pass_len < 8 || pass_len > 63)) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) return ESP_ERR_NO_MEM;

    cJSON_AddStringToObject(root, "ssid", config->ssid);
    cJSON_AddStringToObject(root, "password", config->password);
    cJSON_AddStringToObject(root, "country_code", config->country_code);
    cJSON_AddStringToObject(root, "wps_device_name", config->wps_device_name);
    cJSON_AddBoolToObject(root, "wps_enabled", config->wps_enabled);
    cJSON_AddBoolToObject(root, "apname_ie_enabled", config->apname_ie_enabled);
    cJSON_AddNumberToObject(root, "channel", config->channel);
    cJSON_AddNumberToObject(root, "bandwidth", config->bandwidth);
    cJSON_AddNumberToObject(root, "max_connection", config->max_connection);
    cJSON_AddNumberToObject(root, "gtk_rekey_interval", config->gtk_rekey_interval);
    
    // Save auth_mode as string for readability
    const char *auth_str = "WPA2_PSK";
    if (config->auth_mode == WIFI_AUTH_WPA3_PSK) {
        auth_str = "WPA3_PSK";
    }
    cJSON_AddStringToObject(root, "auth_mode", auth_str);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return ESP_ERR_NO_MEM;

    nvs_handle_t h;
    esp_err_t err = nvs_open(SOFTAP_CONFIG_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        free(json);
        return err;
    }
    err = nvs_set_str(h, "json", json);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    free(json);
    return err;
}

void softap_config_generate_default(softap_config_t *out_config)
{
    if (!out_config) return;
    // Generation rules (assumptions):
    // - SSID: "ESP32-" + last 3 bytes of MAC
    // - Password: empty (open) if a compile-time flag allows, else random-ish based on MAC
    // - Channel: 1, 6, or 11 based on MAC address (for better distribution)
    // - max_connection: 4
    // - gtk_rekey_interval: 0

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);

    // Format: CYBR3720-ESP32-XXXX where XXXX are the last 4 hex digits of the MAC
    // Use template from .env (DEFAULT_SSID_PREFIX_TEMPLATE), replacing XXXX with last two MAC bytes
    const char *tpl = DEFAULT_SSID_PREFIX_TEMPLATE;
    char tmp[64];
    const char *pos = strstr(tpl, "XXXX");
    if (pos) {
        // build prefix around XXXX
        size_t prefix_len = pos - tpl;
        if (prefix_len >= sizeof(tmp)) prefix_len = sizeof(tmp) - 1;
        memcpy(tmp, tpl, prefix_len);
        int n = snprintf(tmp + prefix_len, sizeof(tmp) - prefix_len, "%02X%02X", mac[4], mac[5]);
        tmp[prefix_len + n] = '\0';
        strncpy(out_config->ssid, tmp, sizeof(out_config->ssid)-1);
    } else {
        // fallback: append last two bytes
        snprintf(out_config->ssid, sizeof(out_config->ssid), "%s%02X%02X", tpl, mac[4], mac[5]);
    }

    // By default make an open network if allowed by Kconfig, else derive a simple password
#ifdef CONFIG_ALLOW_OPEN_SOFTAP
    out_config->password[0] = '\0';
    out_config->auth_open = true;
#else
    snprintf(out_config->password, sizeof(out_config->password), "AP%02X%02X%02X", mac[3], mac[4], mac[5]);
    out_config->auth_open = (strlen(out_config->password) == 0);
#endif

    // Set default auth mode from environment variable
#ifdef DEFAULT_AUTH_MODE
    const char *default_auth = DEFAULT_AUTH_MODE;
    if (strcmp(default_auth, "WPA3_PSK") == 0 || strcmp(default_auth, "WPA3-SAE") == 0) {
        out_config->auth_mode = WIFI_AUTH_WPA3_PSK;
    } else {
        out_config->auth_mode = WIFI_AUTH_WPA2_PSK;
    }
#else
    out_config->auth_mode = WIFI_AUTH_WPA2_PSK;
#endif

    // Set default country code from environment variable
#ifdef DEFAULT_COUNTRY_CODE
    strncpy(out_config->country_code, DEFAULT_COUNTRY_CODE, sizeof(out_config->country_code)-1);
    out_config->country_code[sizeof(out_config->country_code)-1] = '\0';
#else
    memcpy(out_config->country_code, "US", 2);
    out_config->country_code[2] = '\0';
#endif

    // Set default WPS device name from environment variable
    // If not specified, use the same template as SSID
#ifdef DEFAULT_WPS_DEVICE_NAME
    const char *wps_tpl = DEFAULT_WPS_DEVICE_NAME;
    const char *wps_pos = strstr(wps_tpl, "XXXX");
    if (wps_pos) {
        // build prefix around XXXX
        size_t wps_prefix_len = wps_pos - wps_tpl;
        if (wps_prefix_len >= sizeof(tmp)) wps_prefix_len = sizeof(tmp) - 1;
        memcpy(tmp, wps_tpl, wps_prefix_len);
        int n = snprintf(tmp + wps_prefix_len, sizeof(tmp) - wps_prefix_len, "%02X%02X", mac[4], mac[5]);
        tmp[wps_prefix_len + n] = '\0';
        strncpy(out_config->wps_device_name, tmp, sizeof(out_config->wps_device_name)-1);
    } else {
        // fallback: append last two bytes
        snprintf(out_config->wps_device_name, sizeof(out_config->wps_device_name), "%s%02X%02X", wps_tpl, mac[4], mac[5]);
    }
#else
    // Default to SSID if not specified
    strncpy(out_config->wps_device_name, out_config->ssid, sizeof(out_config->wps_device_name)-1);
#endif

    // Set default WPS and AP Name IE enabled flags from environment variables
#ifdef DEFAULT_WPS_ENABLED
    out_config->wps_enabled = (strcmp(DEFAULT_WPS_ENABLED, "true") == 0 || strcmp(DEFAULT_WPS_ENABLED, "1") == 0);
#else
    out_config->wps_enabled = true; // default: enabled
#endif

#ifdef DEFAULT_APNAME_IE_ENABLED
    out_config->apname_ie_enabled = (strcmp(DEFAULT_APNAME_IE_ENABLED, "true") == 0 || strcmp(DEFAULT_APNAME_IE_ENABLED, "1") == 0);
#else
    out_config->apname_ie_enabled = true; // default: enabled
#endif

    // Select channel based on MAC address for better distribution
    // Use last byte modulo 3 to map to channels 1, 6, or 11
    // These are the three non-overlapping channels in 2.4GHz band
    const int channels[] = {1, 6, 11};
    int channel_index = mac[5] % 3;
    out_config->channel = channels[channel_index];

    out_config->bandwidth = WIFI_BW_HT20;
    out_config->max_connection = 4;
    out_config->gtk_rekey_interval = 0;
}
