/**
 * @file softap_webserver.c
 * @brief Web server implementation for BeaconBit configuration
 */

#include "softap_webserver.h"
#include "softap_config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "beaconbit_webserver";
static httpd_handle_t server = NULL;

/* HTML page header */
static const char html_header[] =
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>BeaconBit Configuration</title>"
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
    "<h1>BeaconBit Configuration</h1>";

static const char html_footer[] =
    "<div class='footer'>"
    "<p>BeaconBit Web Interface | Configuration stored in NVS</p>"
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
    beaconbit_config_t config;
    esp_err_t ret = beaconbit_config_load(&config);

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
        "<tr><td class='label'>WPS Device Name</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>WPS Enabled</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>AP Name IE Value</td><td class='value'>%s</td></tr>"
        "<tr><td class='label'>AP Name IE Enabled</td><td class='value'>%s</td></tr>"
        "</table>",
        config.ssid,
        (strlen(config.password) > 0) ? "********" : "(none - open network)",
        config.country_code,
        config.channel,
        bandwidth_to_string(config.bandwidth),
        config.max_connection,
        auth_mode_to_string(config.auth_mode),
        config.gtk_rekey_interval,
        config.wps_device_name,
        config.wps_enabled ? "Yes" : "No",
        config.apname_ie_value,
        config.apname_ie_enabled ? "Yes" : "No"
    );

    httpd_resp_send_chunk(req, buffer, len);

    /* Navigation section */
    const char nav_section[] =
        "<h2>Tools</h2>"
        "<div class='info-box'>"
        "<a href='/speedtest' style='color: #2196F3; text-decoration: none; font-weight: bold;'>"
        "⚡ Speed Test - Test your WiFi performance</a>"
        "</div>";
    httpd_resp_send_chunk(req, nav_section, strlen(nav_section));

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
    beaconbit_config_t config;
    esp_err_t ret = beaconbit_config_load(&config);

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
    cJSON_AddStringToObject(root, "wps_device_name", config.wps_device_name);
    cJSON_AddBoolToObject(root, "wps_enabled", config.wps_enabled);
    cJSON_AddBoolToObject(root, "apname_ie_enabled", config.apname_ie_enabled);
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
 * @brief State for speed test to prevent concurrent tests
 */
static bool speedtest_in_progress = false;

/**
 * @brief Handler for GET /api/speedtest/download - streams data for download test
 */
static esp_err_t speedtest_download_handler(httpd_req_t *req) {
    if (speedtest_in_progress) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send_err(req, HTTPD_503_SERVICE_UNAVAILABLE,
                           "{\"error\":\"Test already in progress\"}");
        return ESP_FAIL;
    }

    speedtest_in_progress = true;
    ESP_LOGI(TAG, "Starting download speed test");

    // Set chunked encoding
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");

    // Allocate buffer for test data (8KB chunks)
    const size_t chunk_size = 8192;
    uint8_t *buffer = malloc(chunk_size);
    if (buffer == NULL) {
        speedtest_in_progress = false;
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    // Fill buffer with pattern data (more efficient than random)
    for (size_t i = 0; i < chunk_size; i++) {
        buffer[i] = (uint8_t)(i & 0xFF);
    }

    // Stream data for 10 seconds
    int64_t start_time = esp_timer_get_time();
    int64_t duration_us = 10 * 1000000;  // 10 seconds
    size_t total_sent = 0;
    esp_err_t ret = ESP_OK;

    while ((esp_timer_get_time() - start_time) < duration_us) {
        ret = httpd_resp_send_chunk(req, (char*)buffer, chunk_size);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Download test failed to send chunk");
            break;
        }
        total_sent += chunk_size;

        // Small delay to prevent overwhelming the network stack
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // End chunked response
    httpd_resp_send_chunk(req, NULL, 0);

    free(buffer);
    speedtest_in_progress = false;

    int64_t elapsed_us = esp_timer_get_time() - start_time;
    float mbps = (total_sent * 8.0f) / (elapsed_us / 1000000.0f) / 1000000.0f;
    ESP_LOGI(TAG, "Download test complete: %zu bytes in %.2f sec = %.2f Mbps",
             total_sent, elapsed_us / 1000000.0f, mbps);

    return ret;
}

/**
 * @brief Handler for POST /api/speedtest/upload - receives data for upload test
 */
static esp_err_t speedtest_upload_handler(httpd_req_t *req) {
    if (speedtest_in_progress) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send_err(req, HTTPD_503_SERVICE_UNAVAILABLE,
                           "{\"error\":\"Test already in progress\"}");
        return ESP_FAIL;
    }

    speedtest_in_progress = true;
    ESP_LOGI(TAG, "Starting upload speed test");

    // Buffer for receiving data
    const size_t buf_size = 8192;
    char *buffer = malloc(buf_size);
    if (buffer == NULL) {
        speedtest_in_progress = false;
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    int64_t start_time = esp_timer_get_time();
    size_t total_received = 0;
    int ret;

    // Receive all data from client
    while (1) {
        ret = httpd_req_recv(req, buffer, buf_size);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            break;
        }
        total_received += ret;
    }

    int64_t elapsed_us = esp_timer_get_time() - start_time;
    float elapsed_sec = elapsed_us / 1000000.0f;
    float mbps = (total_received * 8.0f) / elapsed_sec / 1000000.0f;

    ESP_LOGI(TAG, "Upload test complete: %zu bytes in %.2f sec = %.2f Mbps",
             total_received, elapsed_sec, mbps);

    // Send results as JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "bytes", total_received);
    cJSON_AddNumberToObject(root, "duration", elapsed_sec);
    cJSON_AddNumberToObject(root, "mbps", mbps);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str != NULL) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_str, strlen(json_str));
        free(json_str);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON error");
    }

    free(buffer);
    speedtest_in_progress = false;

    return ESP_OK;
}

/**
 * @brief Handler for GET /api/speedtest/iperf - returns iperf server status
 */
static esp_err_t speedtest_iperf_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();

#ifdef CONFIG_BEACONBIT_IPERF_SERVER_ENABLE
    cJSON_AddBoolToObject(root, "enabled", true);
    cJSON_AddNumberToObject(root, "port", CONFIG_BEACONBIT_IPERF_SERVER_PORT);
    cJSON_AddStringToObject(root, "command", "iperf -c 192.168.4.1 -i 1 -t 30");
#else
    cJSON_AddBoolToObject(root, "enabled", false);
    cJSON_AddStringToObject(root, "message", "iperf server not enabled in build");
#endif

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON error");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);

    return ESP_OK;
}

/**
 * @brief Handler for GET /speedtest - speed test page
 */
static esp_err_t speedtest_page_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");

    /* Send header */
    httpd_resp_send_chunk(req, html_header, strlen(html_header));

    /* Speedtest page HTML with embedded JavaScript */
    const char speedtest_html[] =
        "<style>"
        ".tab-container { margin: 20px 0; }"
        ".tab-buttons { display: flex; border-bottom: 2px solid #4CAF50; }"
        ".tab-button { flex: 1; padding: 12px; background: #f5f5f5; border: none; cursor: pointer; font-size: 16px; font-weight: bold; transition: background 0.3s; }"
        ".tab-button.active { background: #4CAF50; color: white; }"
        ".tab-button:hover { background: #45a049; color: white; }"
        ".tab-content { display: none; padding: 20px; }"
        ".tab-content.active { display: block; }"
        ".test-button { background: #2196F3; margin: 10px 5px; }"
        ".test-button:hover { background: #0b7dda; }"
        ".test-button:disabled { background: #ccc; }"
        ".progress-bar { width: 100%; height: 30px; background: #f0f0f0; border-radius: 15px; overflow: hidden; margin: 15px 0; }"
        ".progress-fill { height: 100%; background: linear-gradient(90deg, #4CAF50, #45a049); transition: width 0.3s; text-align: center; line-height: 30px; color: white; font-weight: bold; }"
        ".result-box { background: #e8f5e9; border-left: 4px solid #4CAF50; padding: 15px; margin: 15px 0; font-size: 18px; }"
        ".result-box .speed { font-size: 32px; font-weight: bold; color: #2e7d32; }"
        ".code-box { background: #263238; color: #aed581; padding: 15px; border-radius: 4px; font-family: monospace; margin: 10px 0; }"
        ".copy-btn { background: #607d8b; padding: 5px 15px; margin-left: 10px; font-size: 14px; }"
        ".copy-btn:hover { background: #455a64; }"
        ".status-indicator { display: inline-block; width: 12px; height: 12px; border-radius: 50%; margin-right: 8px; }"
        ".status-indicator.on { background: #4CAF50; box-shadow: 0 0 8px #4CAF50; }"
        ".status-indicator.off { background: #f44336; }"
        ".nav-link { display: inline-block; margin: 10px 0; color: #2196F3; text-decoration: none; font-weight: bold; }"
        ".nav-link:hover { text-decoration: underline; }"
        "</style>"
        "<a href='/' class='nav-link'>← Back to Configuration</a>"
        "<h2>Speed Test</h2>"
        "<div class='tab-container'>"
        "<div class='tab-buttons'>"
        "<button class='tab-button active' onclick='showTab(0)'>Quick Test</button>"
        "<button class='tab-button' onclick='showTab(1)'>Advanced (iperf)</button>"
        "</div>"
        "<div class='tab-content active' id='tab0'>"
        "<div class='info-box'>"
        "<strong>Browser-Based Speed Test</strong><br>"
        "Test your WiFi connection speed directly from your browser. No additional software required!"
        "</div>"
        "<button class='btn test-button' id='downloadBtn' onclick='runDownloadTest()'>Download Test</button>"
        "<button class='btn test-button' id='uploadBtn' onclick='runUploadTest()'>Upload Test</button>"
        "<div id='progress' style='display:none;'>"
        "<div class='progress-bar'><div class='progress-fill' id='progressBar'>0%</div></div>"
        "<p id='progressText'>Preparing test...</p>"
        "</div>"
        "<div id='results'></div>"
        "</div>"
        "<div class='tab-content' id='tab1'>"
        "<div class='info-box'>"
        "<strong>iperf Server</strong><br>"
        "For more accurate bandwidth testing, use the built-in iperf server with a standard iperf client."
        "</div>"
        "<div id='iperfStatus'>Loading...</div>"
        "<div id='iperfInstructions'></div>"
        "</div>"
        "</div>"
        "<script>"
        "let testRunning=false;"
        "function showTab(n){"
        "document.querySelectorAll('.tab-button').forEach((b,i)=>b.className=i===n?'tab-button active':'tab-button');"
        "document.querySelectorAll('.tab-content').forEach((c,i)=>c.className=i===n?'tab-content active':'tab-content');"
        "if(n===1)loadIperfStatus();"
        "}"
        "function setProgress(pct,text){"
        "document.getElementById('progressBar').style.width=pct+'%';"
        "document.getElementById('progressBar').textContent=pct+'%';"
        "document.getElementById('progressText').textContent=text;"
        "}"
        "function showResult(type,mbps,duration){"
        "const r=document.getElementById('results');"
        "r.innerHTML='<div class=\"result-box\"><strong>'+type+' Test Complete</strong><br><div class=\"speed\">'+mbps.toFixed(2)+' Mbps</div><small>Duration: '+duration.toFixed(2)+' seconds</small></div>'+r.innerHTML;"
        "}"
        "async function runDownloadTest(){"
        "if(testRunning)return;"
        "testRunning=true;"
        "const btn=document.getElementById('downloadBtn');"
        "btn.disabled=true;"
        "document.getElementById('uploadBtn').disabled=true;"
        "document.getElementById('progress').style.display='block';"
        "setProgress(0,'Starting download test...');"
        "try{"
        "const start=Date.now();"
        "let received=0;"
        "const resp=await fetch('/api/speedtest/download');"
        "const reader=resp.body.getReader();"
        "let lastUpdate=start;"
        "while(true){"
        "const{done,value}=await reader.read();"
        "if(done)break;"
        "received+=value.length;"
        "const now=Date.now();"
        "const elapsed=(now-start)/1000;"
        "const pct=Math.min(95,elapsed*10);"
        "if(now-lastUpdate>200){"
        "const mbps=(received*8)/(elapsed*1000000);"
        "setProgress(pct,'Downloading... '+mbps.toFixed(2)+' Mbps');"
        "lastUpdate=now;"
        "}"
        "}"
        "const duration=(Date.now()-start)/1000;"
        "const mbps=(received*8)/(duration*1000000);"
        "setProgress(100,'Complete!');"
        "showResult('Download',mbps,duration);"
        "}catch(e){"
        "alert('Download test failed: '+e.message);"
        "}finally{"
        "testRunning=false;"
        "btn.disabled=false;"
        "document.getElementById('uploadBtn').disabled=false;"
        "setTimeout(()=>document.getElementById('progress').style.display='none',2000);"
        "}"
        "}"
        "async function runUploadTest(){"
        "if(testRunning)return;"
        "testRunning=true;"
        "const btn=document.getElementById('uploadBtn');"
        "btn.disabled=true;"
        "document.getElementById('downloadBtn').disabled=true;"
        "document.getElementById('progress').style.display='block';"
        "setProgress(0,'Starting upload test...');"
        "try{"
        "const size=5*1024*1024;"
        "const data=new Uint8Array(size);"
        "for(let i=0;i<size;i++)data[i]=i&0xFF;"
        "setProgress(20,'Uploading data...');"
        "const start=Date.now();"
        "const resp=await fetch('/api/speedtest/upload',{method:'POST',body:data});"
        "const result=await resp.json();"
        "setProgress(100,'Complete!');"
        "showResult('Upload',result.mbps,result.duration);"
        "}catch(e){"
        "alert('Upload test failed: '+e.message);"
        "}finally{"
        "testRunning=false;"
        "btn.disabled=false;"
        "document.getElementById('downloadBtn').disabled=false;"
        "setTimeout(()=>document.getElementById('progress').style.display='none',2000);"
        "}"
        "}"
        "async function loadIperfStatus(){"
        "try{"
        "const resp=await fetch('/api/speedtest/iperf');"
        "const data=await resp.json();"
        "const status=document.getElementById('iperfStatus');"
        "const instructions=document.getElementById('iperfInstructions');"
        "if(data.enabled){"
        "status.innerHTML='<p><span class=\"status-indicator on\"></span><strong>iperf server is running</strong></p><p>Port: '+data.port+'</p>';"
        "instructions.innerHTML='<h3>How to test:</h3><div class=\"info-box\"><strong>1. Install iperf client</strong><br>Linux/Mac: <code>sudo apt install iperf</code> or <code>brew install iperf</code><br>Windows: Download from <a href=\"https://iperf.fr/iperf-download.php\" target=\"_blank\">iperf.fr</a></div><div class=\"info-box\"><strong>2. Run test from your computer</strong><br><div class=\"code-box\">$ '+data.command+'<button class=\"btn copy-btn\" onclick=\"navigator.clipboard.writeText(\\''+data.command+'\\');alert(\\'Copied!\\');\">Copy</button></div></div>';"
        "}else{"
        "status.innerHTML='<p><span class=\"status-indicator off\"></span><strong>iperf server not enabled</strong></p>';"
        "instructions.innerHTML='<p>'+data.message+'</p>';"
        "}"
        "}catch(e){"
        "document.getElementById('iperfStatus').innerHTML='<p>Error loading status</p>';"
        "}"
        "}"
        "</script>";

    httpd_resp_send_chunk(req, speedtest_html, strlen(speedtest_html));

    /* Send footer */
    httpd_resp_send_chunk(req, html_footer, strlen(html_footer));

    /* Signal end of response */
    httpd_resp_send_chunk(req, NULL, 0);

    ESP_LOGI(TAG, "Speedtest page served");
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
    beaconbit_config_t config;
    esp_err_t err = beaconbit_config_load(&config);
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
    err = beaconbit_config_save(&config);
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

/* URI handler for speedtest page */
static const httpd_uri_t speedtest_uri = {
    .uri       = "/speedtest",
    .method    = HTTP_GET,
    .handler   = speedtest_page_handler,
    .user_ctx  = NULL
};

/* URI handler for speedtest download endpoint */
static const httpd_uri_t speedtest_download_uri = {
    .uri       = "/api/speedtest/download",
    .method    = HTTP_GET,
    .handler   = speedtest_download_handler,
    .user_ctx  = NULL
};

/* URI handler for speedtest upload endpoint */
static const httpd_uri_t speedtest_upload_uri = {
    .uri       = "/api/speedtest/upload",
    .method    = HTTP_POST,
    .handler   = speedtest_upload_handler,
    .user_ctx  = NULL
};

/* URI handler for speedtest iperf status endpoint */
static const httpd_uri_t speedtest_iperf_uri = {
    .uri       = "/api/speedtest/iperf",
    .method    = HTTP_GET,
    .handler   = speedtest_iperf_handler,
    .user_ctx  = NULL
};

esp_err_t beaconbit_webserver_start(void) {
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
    httpd_register_uri_handler(server, &speedtest_uri);
    httpd_register_uri_handler(server, &speedtest_download_uri);
    httpd_register_uri_handler(server, &speedtest_upload_uri);
    httpd_register_uri_handler(server, &speedtest_iperf_uri);

    ESP_LOGI(TAG, "Web server started successfully");
    ESP_LOGI(TAG, "Access the configuration page at http://<ESP32_IP>/");
    ESP_LOGI(TAG, "Access the speed test page at http://<ESP32_IP>/speedtest");

    return ESP_OK;
}

esp_err_t beaconbit_webserver_stop(void) {
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