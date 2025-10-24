/**
 * @file softap_webserver.c
 * @brief Web server implementation for SoftAP configuration
 */

#include "softap_webserver.h"
#include "softap_config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "softap_webserver";
static httpd_handle_t server = NULL;

/* HTML page header */
static const char html_header[] =
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>ESP32 SoftAP Configuration</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; margin: 40px; background-color: #f5f5f5; }"
    "h1 { color: #333; }"
    "h2 { color: #666; margin-top: 30px; }"
    ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
    ".config-table { width: 100%; border-collapse: collapse; margin-top: 20px; }"
    ".config-table th, .config-table td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }"
    ".config-table th { background-color: #4CAF50; color: white; font-weight: bold; }"
    ".config-table tr:hover { background-color: #f5f5f5; }"
    ".label { font-weight: bold; color: #555; }"
    ".value { color: #333; font-family: monospace; }"
    ".footer { margin-top: 30px; text-align: center; color: #999; font-size: 0.9em; }"
    ".info-box { background-color: #e7f3ff; border-left: 4px solid #2196F3; padding: 15px; margin: 20px 0; }"
    "</style>"
    "</head>"
    "<body>"
    "<div class='container'>"
    "<h1>ESP32 SoftAP Configuration</h1>";

static const char html_footer[] =
    "<div class='footer'>"
    "<p>ESP32 SoftAP Web Interface | Configuration stored in NVS</p>"
    "</div>"
    "</div>"
    "</body>"
    "</html>";

/**
 * @brief Convert auth mode enum to human-readable string
 */
static const char* auth_mode_to_string(uint8_t auth_mode) {
    switch (auth_mode) {
        case WIFI_AUTH_OPEN:
            return "Open (No Password)";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2-PSK";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3-PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3-PSK";
        default:
            return "Unknown";
    }
}

/**
 * @brief Convert bandwidth enum to human-readable string
 */
static const char* bandwidth_to_string(uint8_t bandwidth) {
    switch (bandwidth) {
        case WIFI_BW_HT20:
            return "20 MHz";
        case WIFI_BW_HT40:
            return "40 MHz";
        default:
            return "Unknown";
    }
}

/**
 * @brief Handler for root path - displays current configuration
 */
static esp_err_t root_get_handler(httpd_req_t *req) {
    softap_config_t config;
    esp_err_t ret = softap_config_load(&config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load config for display: %s", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to load configuration");
        return ESP_FAIL;
    }

    /* Start building HTML response */
    httpd_resp_set_type(req, "text/html");

    /* Send header */
    httpd_resp_send_chunk(req, html_header, strlen(html_header));

    /* Info box */
    const char info_box[] =
        "<div class='info-box'>"
        "<strong>Note:</strong> This page displays the current configuration stored in NVS. "
        "Configuration editing will be available in a future update."
        "</div>";
    httpd_resp_send_chunk(req, info_box, strlen(info_box));

    /* Build configuration table */
    char buffer[1024];
    int len;

    len = snprintf(buffer, sizeof(buffer),
        "<h2>Current Configuration</h2>"
        "<table class='config-table'>"
        "<tr><th>Setting</th><th>Value</th></tr>"
        "<tr><td class='label'>SSID</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>Password</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>Channel</td><td class='value'>%d</td></tr>"
        "<tr><td class='label'>Bandwidth</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>Max Connections</td><td class='value'>%d</td></tr>"
        "<tr><td class='label'>Authentication Mode</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>GTK Rekey Interval</td><td class='value'>%d seconds</td></tr>"
        "</table>",
        config.ssid,
        (strlen(config.password) > 0) ? "********" : "(none - open network)",
        config.channel,
        bandwidth_to_string(config.bandwidth),
        config.max_connection,
        auth_mode_to_string(config.auth_mode),
        config.gtk_rekey_interval
    );

    httpd_resp_send_chunk(req, buffer, len);

    /* Send footer */
    httpd_resp_send_chunk(req, html_footer, strlen(html_footer));

    /* Signal end of response */
    httpd_resp_send_chunk(req, NULL, 0);

    ESP_LOGI(TAG, "Configuration page served");
    return ESP_OK;
}

/**
 * @brief Handler for /api/config - returns configuration as JSON
 */
static esp_err_t api_config_get_handler(httpd_req_t *req) {
    softap_config_t config;
    esp_err_t ret = softap_config_load(&config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load config for API: %s", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to load configuration");
        return ESP_FAIL;
    }

    /* Build JSON response */
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create JSON");
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(root, "ssid", config.ssid);
    cJSON_AddBoolToObject(root, "has_password", strlen(config.password) > 0);
    cJSON_AddNumberToObject(root, "channel", config.channel);
    cJSON_AddNumberToObject(root, "bandwidth", config.bandwidth);
    cJSON_AddNumberToObject(root, "max_connection", config.max_connection);
    cJSON_AddNumberToObject(root, "auth_mode", config.auth_mode);
    cJSON_AddNumberToObject(root, "gtk_rekey_interval", config.gtk_rekey_interval);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to serialize JSON");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    free(json_str);

    ESP_LOGI(TAG, "API config served");
    return ESP_OK;
}

/* URI handler for root */
static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

/* URI handler for API config endpoint */
static const httpd_uri_t api_config_uri = {
    .uri       = "/api/config",
    .method    = HTTP_GET,
    .handler   = api_config_get_handler,
    .user_ctx  = NULL
};

esp_err_t softap_webserver_start(void) {
    if (server != NULL) {
        ESP_LOGW(TAG, "Web server already started");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting web server on port %d", config.server_port);

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register URI handlers */
    httpd_register_uri_handler(server, &root_uri);
    httpd_register_uri_handler(server, &api_config_uri);

    ESP_LOGI(TAG, "Web server started successfully");
    ESP_LOGI(TAG, "Access the configuration page at http://<ESP32_IP>/");

    return ESP_OK;
}

esp_err_t softap_webserver_stop(void) {
    if (server == NULL) {
        ESP_LOGW(TAG, "Web server not running");
        return ESP_OK;
    }

    esp_err_t ret = httpd_stop(server);
    if (ret == ESP_OK) {
        server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    } else {
        ESP_LOGE(TAG, "Failed to stop web server: %s", esp_err_to_name(ret));
    }

    return ret;
}