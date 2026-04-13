#pragma once

#include <esp_err.h>
#include <esp_http_server.h>
#include <functional>

extern httpd_handle_t g_webserver;

/**
 * Registers event handlers to network connections WiFi and/or Ethernet which start the webserver if successfully created a connection.
 *
 * @retval - `ESP_OK`: Succeed
 * @retval - `ESP_ERR_INVALID_STATE`: Neither WiFi nor Ethernet are enabled in menuconfig.
 * @retval - `ESP_ERR_NO_MEM`: Cannot allocate memory for the handler
 * @retval - `ESP_ERR_INVALID_ARG`: Invalid combination of event base and event ID
 *
 * For more details, see:
 *
 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
 *
 * - [ESP-IDF WiFi Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/wifi-driver/station-scenarios.html)
 *
 * - [ESP-IDF Ethernet Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/network/esp_eth.html)
 */
esp_err_t init_webserver(const std::function<void(httpd_handle_t)> &state_callback);