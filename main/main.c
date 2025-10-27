#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
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

static const char *TAG = "wifi softAP";

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
    }
}

void wifi_init_softap(void)
{
    softap_config_t cfg;
    esp_err_t r = softap_config_load(&cfg);
    if (r == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No softAP config found in NVS; generating default");
        softap_config_generate_default(&cfg);
        esp_err_t s = softap_config_save(&cfg);
        if (s != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save generated softAP config: %s", esp_err_to_name(s));
        } else {
            ESP_LOGI(TAG, "Generated softAP config saved to NVS");
        }
    } else if (r != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load softAP config (%s), using defaults", esp_err_to_name(r));
        softap_config_generate_default(&cfg);
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
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // The password being shown in plaintext to the console is an acceptable security risk for this application.
    const char *auth_mode_str = "UNKNOWN";
    switch (wifi_config.ap.authmode) {
        case WIFI_AUTH_OPEN:            auth_mode_str = "OPEN"; break;
        case WIFI_AUTH_WPA2_PSK:        auth_mode_str = "WPA2-PSK"; break;
        case WIFI_AUTH_WPA3_PSK:        auth_mode_str = "WPA3-PSK"; break;
        default:                        auth_mode_str = "OTHER"; break;
    }
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d auth:%s",
             cfg.ssid, cfg.password, cfg.channel, auth_mode_str);
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
    ret = softap_webserver_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}
