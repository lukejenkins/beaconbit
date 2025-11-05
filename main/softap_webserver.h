/**
 * @file softap_webserver.h
 * @brief Web server for BeaconBit configuration interface
 *
 * Provides a simple HTTP server that displays and allows modification
 * of the access point configuration through a web interface.
 */

#ifndef BEACONBIT_WEBSERVER_H
#define BEACONBIT_WEBSERVER_H

#include "esp_http_server.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the HTTP web server
 *
 * Starts the HTTP server on port 80 and registers all URI handlers
 * for the configuration interface.
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_* on failure
 */
esp_err_t beaconbit_webserver_start(void);

/**
 * @brief Stop the HTTP web server
 *
 * Stops the running HTTP server and frees all resources.
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_* on failure
 */
esp_err_t beaconbit_webserver_stop(void);

#ifdef __cplusplus
}
#endif

#endif // BEACONBIT_WEBSERVER_H