#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "sdkconfig.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "softap_config.h"
#include "softap_webserver.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#if CONFIG_ESP_GTK_REKEYING_ENABLE
#define EXAMPLE_GTK_REKEY_INTERVAL CONFIG_ESP_GTK_REKEY_INTERVAL
#else
#define EXAMPLE_GTK_REKEY_INTERVAL 0
#endif

static const char *TAG = "beaconbit";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    } else if (event_id == WIFI_EVENT_AP_WPS_RG_SUCCESS) {
        wifi_event_ap_wps_rg_success_t *evt = (wifi_event_ap_wps_rg_success_t *)event_data;
        ESP_LOGI(TAG, "WPS: station "MACSTR" WPS successful",
                 MAC2STR(evt->peer_macaddr));
    } else if (event_id == WIFI_EVENT_AP_WPS_RG_FAILED) {
        wifi_event_ap_wps_rg_fail_reason_t *evt = (wifi_event_ap_wps_rg_fail_reason_t *)event_data;
        ESP_LOGI(TAG, "WPS: station "MACSTR" WPS failed, reason=%d",
                 MAC2STR(evt->peer_macaddr), evt->reason);
    } else if (event_id == WIFI_EVENT_AP_WPS_RG_TIMEOUT) {
        ESP_LOGI(TAG, "WPS: registration timeout");
    } else if (event_id == WIFI_EVENT_AP_WPS_RG_PIN) {
        wifi_event_ap_wps_rg_pin_t* event = (wifi_event_ap_wps_rg_pin_t *) event_data;
        ESP_LOGI(TAG, "WPS: PIN = %c%c%c%c%c%c%c%c",
                 event->pin_code[0], event->pin_code[1], event->pin_code[2], event->pin_code[3],
                 event->pin_code[4], event->pin_code[5], event->pin_code[6], event->pin_code[7]);
    }
}

void wifi_init_softap(void)
{
    beaconbit_config_t cfg;
    esp_err_t r = beaconbit_config_load(&cfg);
    if (r == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No config found in NVS; generating default");
        beaconbit_config_generate_default(&cfg);
        esp_err_t s = beaconbit_config_save(&cfg);
        if (s != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save generated config: %s", esp_err_to_name(s));
        } else {
            ESP_LOGI(TAG, "Generated config saved to NVS");
        }
    } else if (r != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config (%s), using defaults", esp_err_to_name(r));
        beaconbit_config_generate_default(&cfg);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wcfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.ap.ssid, cfg.ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(cfg.ssid);
    wifi_config.ap.channel = cfg.channel;
    strncpy((char *)wifi_config.ap.password, cfg.password, sizeof(wifi_config.ap.password));
    wifi_config.ap.max_connection = cfg.max_connection;
    
    // Note: WPS device name is stored in NVS and displayed in web UI, but ESP-IDF's
    // wifi_ap_config_t doesn't have a direct field for it. The device name is primarily
    // used for display/documentation purposes in the web interface.
    
    // Use auth_mode from configuration (NVS), but verify WPA3 support is available
    if (cfg.auth_mode == WIFI_AUTH_WPA3_PSK) {
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
        wifi_config.ap.authmode = WIFI_AUTH_WPA3_PSK;
        wifi_config.ap.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
        ESP_LOGI(TAG, "Using WPA3-PSK authentication (SAE support enabled)");
#else
        // Fall back to WPA2 if WPA3 is configured but SAE support isn't compiled in
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        ESP_LOGW(TAG, "WPA3 requested but CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT not enabled; falling back to WPA2-PSK");
#endif
    } else {
        // Use WPA2 by default or as explicitly configured
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        ESP_LOGI(TAG, "Using WPA2-PSK authentication");
    }
    
    wifi_config.ap.pmf_cfg.required = true;
#ifdef CONFIG_ESP_WIFI_BSS_MAX_IDLE_SUPPORT
    wifi_config.ap.bss_max_idle_cfg.period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD;
    wifi_config.ap.bss_max_idle_cfg.protected_keep_alive = 1;
#endif
    wifi_config.ap.gtk_rekey_interval = cfg.gtk_rekey_interval;
    
    // Override auth mode for open networks (no password)
    if (cfg.auth_open || strlen(cfg.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        ESP_LOGI(TAG, "Using OPEN authentication (no password)");
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    // Set channel width from configuration (persisted)
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, cfg.bandwidth));
    
    // Set country code for regulatory domain
    wifi_country_t country = {
        .cc = {cfg.country_code[0], cfg.country_code[1], 0},
        .schan = 1,          // Start channel
        .nchan = 13,         // Number of channels (1-13 for most countries, 1-11 for US/Canada)
        .policy = WIFI_COUNTRY_POLICY_AUTO
    };
    
    // Adjust channel count for specific countries
    if (strcmp(cfg.country_code, "US") == 0 || strcmp(cfg.country_code, "CA") == 0) {
        country.nchan = 11;  // US and Canada use channels 1-11
    } else if (strcmp(cfg.country_code, "JP") == 0) {
        country.nchan = 14;  // Japan can use channels 1-14 (14 is special case)
    }
    
    esp_err_t country_err = esp_wifi_set_country(&country);
    if (country_err == ESP_OK) {
        ESP_LOGI(TAG, "Country code set to: %s (channels %d-%d)", cfg.country_code, country.schan, country.nchan);
    } else {
        ESP_LOGW(TAG, "Failed to set country code %s: %s", cfg.country_code, esp_err_to_name(country_err));
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Optionally add Alcatel-Lucent AP Name vendor-specific IE to beacons
    // This supplements the minimal WPS IE in beacons with our device name
    // Using Alcatel-Lucent OUI (0xDC:08:56) which Wireshark already decodes as "wlan.vs.alcatel.apname"
    if (cfg.apname_ie_enabled) {
        size_t device_name_len = strlen(cfg.wps_device_name);
        size_t total_ie_size = sizeof(vendor_ie_data_t) + device_name_len;
        uint8_t *device_name_ie = (uint8_t *)malloc(total_ie_size);
        
        if (device_name_ie != NULL) {
            vendor_ie_data_t *vie = (vendor_ie_data_t *)device_name_ie;
            vie->element_id = WIFI_VENDOR_IE_ELEMENT_ID; // 0xDD
            vie->length = 4 + device_name_len; // OUI (3) + OUI Type (1) + payload length
            vie->vendor_oui[0] = 0xDC; // Alcatel-Lucent OUI byte 1
            vie->vendor_oui[1] = 0x08; // Alcatel-Lucent OUI byte 2
            vie->vendor_oui[2] = 0x56; // Alcatel-Lucent OUI byte 3
            vie->vendor_oui_type = 0x01; // Type: AP Name (ALCATEL_APNAME)
            memcpy(vie->payload, cfg.wps_device_name, device_name_len);
            
            esp_err_t ie_err = esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, 
                                                       WIFI_VND_IE_ID_1, vie);
            if (ie_err == ESP_OK) {
                ESP_LOGI(TAG, "Alcatel-Lucent AP Name IE added to beacons: %s", cfg.wps_device_name);
            } else {
                ESP_LOGW(TAG, "Failed to add Alcatel-Lucent AP Name IE to beacons: %s", esp_err_to_name(ie_err));
            }
            
            free(device_name_ie);
        } else {
            ESP_LOGW(TAG, "Failed to allocate memory for Alcatel-Lucent AP Name IE");
        }
    } else {
        ESP_LOGI(TAG, "Alcatel-Lucent AP Name IE disabled in configuration");
    }

    // Optionally initialize and start WPS with device name from configuration
    // Optionally initialize and start WPS with device name from configuration
    if (cfg.wps_enabled) {
        esp_wps_config_t wps_config = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);
        
        // Set device name from our configuration
        strncpy(wps_config.factory_info.device_name, cfg.wps_device_name, 
                WPS_MAX_DEVICE_NAME_LEN - 1);
        wps_config.factory_info.device_name[WPS_MAX_DEVICE_NAME_LEN - 1] = '\0';
        
        // Set manufacturer and model information
        strncpy(wps_config.factory_info.manufacturer, "ESPRESSIF", 
                WPS_MAX_MANUFACTURER_LEN - 1);
        strncpy(wps_config.factory_info.model_number, CONFIG_IDF_TARGET, 
                WPS_MAX_MODEL_NUMBER_LEN - 1);
        strncpy(wps_config.factory_info.model_name, "ESP SoftAP", 
                WPS_MAX_MODEL_NAME_LEN - 1);
        
        esp_err_t wps_err = esp_wifi_ap_wps_enable(&wps_config);
        if (wps_err == ESP_OK) {
            ESP_LOGI(TAG, "WPS enabled with device name: %s", cfg.wps_device_name);
            // Start WPS in PBC (Push Button Configuration) mode
            wps_err = esp_wifi_ap_wps_start(NULL);
            if (wps_err == ESP_OK) {
                ESP_LOGI(TAG, "WPS started in PBC mode");
            } else {
                ESP_LOGW(TAG, "Failed to start WPS: %s", esp_err_to_name(wps_err));
            }
        } else {
            ESP_LOGW(TAG, "Failed to enable WPS: %s", esp_err_to_name(wps_err));
        }
    } else {
        ESP_LOGI(TAG, "WPS disabled in configuration");
    }

    // The password being shown in plaintext to the console is an acceptable security risk for this application.
    const char *auth_mode_str = "UNKNOWN";
    switch (wifi_config.ap.authmode) {
        case WIFI_AUTH_OPEN:            auth_mode_str = "OPEN"; break;
        case WIFI_AUTH_WPA2_PSK:        auth_mode_str = "WPA2-PSK"; break;
        case WIFI_AUTH_WPA3_PSK:        auth_mode_str = "WPA3-PSK"; break;
        default:                        auth_mode_str = "OTHER"; break;
    }
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d country:%s auth:%s",
             cfg.ssid, cfg.password, cfg.channel, cfg.country_code, auth_mode_str);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    // Start the web server
    ret = beaconbit_webserver_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}
