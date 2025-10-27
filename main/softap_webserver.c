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
    ".form-group { margin: 15px 0; }"
    ".form-group label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }"
    ".form-group select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; font-size: 14px; }"
    ".form-group small { display: block; margin-top: 5px; color: #666; }"
    ".btn { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }"
    ".btn:hover { background-color: #45a049; }"
    ".btn:disabled { background-color: #ccc; cursor: not-allowed; }"
    ".message { padding: 10px; margin: 10px 0; border-radius: 4px; }"
    ".message.success { background-color: #d4edda; border: 1px solid #c3e6cb; color: #155724; }"
    ".message.error { background-color: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }"
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
        "You can now update the WiFi channel below."
        "</div>";
    httpd_resp_send_chunk(req, info_box, strlen(info_box));

    /* Build configuration table */
    char buffer[2048];
    int len;

    len = snprintf(buffer, sizeof(buffer),
        "<h2>Current Configuration</h2>"
        "<table class='config-table'>"
        "<tr><th>Setting</th><th>Value</th></tr>"
        "<tr><td class='label'>SSID</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>Password</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>Country Code</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>Channel</td><td class='value'>%d</td></tr>"
        "<tr><td class='label'>Bandwidth</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>Max Connections</td><td class='value'>%d</td></tr>"
        "<tr><td class='label'>Authentication Mode</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>GTK Rekey Interval</td><td class='value'>%d seconds</td></tr>"
        "</table>",
        config.ssid,
        (strlen(config.password) > 0) ? "********" : "(none - open network)",
        config.country_code,
        config.channel,
        bandwidth_to_string(config.bandwidth),
        config.max_connection,
        auth_mode_to_string(config.auth_mode),
        config.gtk_rekey_interval
    );

    httpd_resp_send_chunk(req, buffer, len);

    /* Channel selection form */
    len = snprintf(buffer, sizeof(buffer),
        "<h2>Update Configuration</h2>"
        "<div id='message'></div>"
        "<form id='channelForm'>"
        "<div class='form-group'>"
        "<label for='channel'>WiFi Channel</label>"
        "<select id='channel' name='channel'>"
        "<option value='1'%s>1 (2.412 GHz) - Recommended</option>"
        "<option value='2'%s>2 (2.417 GHz)</option>"
        "<option value='3'%s>3 (2.422 GHz)</option>"
        "<option value='4'%s>4 (2.427 GHz)</option>"
        "<option value='5'%s>5 (2.432 GHz)</option>"
        "<option value='6'%s>6 (2.437 GHz) - Recommended</option>"
        "<option value='7'%s>7 (2.442 GHz)</option>"
        "<option value='8'%s>8 (2.447 GHz)</option>"
        "<option value='9'%s>9 (2.452 GHz)</option>"
        "<option value='10'%s>10 (2.457 GHz)</option>"
        "<option value='11'%s>11 (2.462 GHz) - Recommended</option>"
        "<option value='12'%s>12 (2.467 GHz)</option>"
        "<option value='13'%s>13 (2.472 GHz)</option>"
        "</select>"
        "<small>Channels 1, 6, and 11 are recommended as they don't overlap. Device will need to restart for changes to take effect.</small>"
        "</div>"
        "<button type='submit' class='btn' id='submitBtn'>Update Channel</button>"
        "</form>"
        "<script>"
        "document.getElementById('channel').value='%d';"
        "document.getElementById('channelForm').onsubmit=function(e){"
        "e.preventDefault();"
        "var btn=document.getElementById('submitBtn');"
        "btn.disabled=true;"
        "btn.textContent='Updating...';"
        "var channel=parseInt(document.getElementById('channel').value);"
        "fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({channel:channel})})"
        ".then(r=>r.json())"
        ".then(data=>{"
        "var msg=document.getElementById('message');"
        "msg.className='message success';"
        "msg.textContent=data.message||'Configuration updated successfully!';"
        "btn.disabled=false;"
        "btn.textContent='Update Channel';"
        "setTimeout(()=>{location.reload();},2000);"
        "})"
        ".catch(err=>{"
        "var msg=document.getElementById('message');"
        "msg.className='message error';"
        "msg.textContent='Failed to update configuration: '+err.message;"
        "btn.disabled=false;"
        "btn.textContent='Update Channel';"
        "});"
        "};"
        "</script>",
        (config.channel == 1) ? " selected" : "",
        (config.channel == 2) ? " selected" : "",
        (config.channel == 3) ? " selected" : "",
        (config.channel == 4) ? " selected" : "",
        (config.channel == 5) ? " selected" : "",
        (config.channel == 6) ? " selected" : "",
        (config.channel == 7) ? " selected" : "",
        (config.channel == 8) ? " selected" : "",
        (config.channel == 9) ? " selected" : "",
        (config.channel == 10) ? " selected" : "",
        (config.channel == 11) ? " selected" : "",
        (config.channel == 12) ? " selected" : "",
        (config.channel == 13) ? " selected" : "",
        config.channel
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
    cJSON_AddStringToObject(root, "country_code", config.country_code);
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

/**
 * @brief Handler for POST /api/config - updates configuration
 */
static esp_err_t api_config_post_handler(httpd_req_t *req) {
    char content[512];
    int ret;
    int remaining = req->content_len;

    if (remaining >= sizeof(content)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too large");
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, content, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[ret] = '\0';

    // Parse JSON
    cJSON *root = cJSON_Parse(content);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Load current config
    softap_config_t config;
    esp_err_t err = softap_config_load(&config);
    if (err != ESP_OK) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to load current config");
        return ESP_FAIL;
    }

    // Update channel if provided
    const cJSON *jchannel = cJSON_GetObjectItemCaseSensitive(root, "channel");
    if (cJSON_IsNumber(jchannel)) {
        int new_channel = jchannel->valueint;
        // Validate channel (1-13 for 2.4GHz, but we recommend 1, 6, 11)
        if (new_channel >= 1 && new_channel <= 13) {
            config.channel = new_channel;
            ESP_LOGI(TAG, "Updated channel to %d", config.channel);
        } else {
            cJSON_Delete(root);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid channel (must be 1-13)");
            return ESP_FAIL;
        }
    }

    cJSON_Delete(root);

    // Save updated config
    err = softap_config_save(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration");
        return ESP_FAIL;
    }

    // Send success response
    httpd_resp_set_type(req, "application/json");
    const char *resp = "{\"success\":true,\"message\":\"Configuration updated. Restart required for changes to take effect.\"}";
    httpd_resp_send(req, resp, strlen(resp));

    ESP_LOGI(TAG, "Configuration updated successfully");
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

/* URI handler for API config POST endpoint */
static const httpd_uri_t api_config_post_uri = {
    .uri       = "/api/config",
    .method    = HTTP_POST,
    .handler   = api_config_post_handler,
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
    httpd_register_uri_handler(server, &api_config_post_uri);

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