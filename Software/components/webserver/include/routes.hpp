#pragma once
#include <esp_err.h>
#include <esp_https_server.h>

/**
 * Registers all routes for the https server
 *
 * @param handle handle to HTTPD server instance
 * @retval - `ESP_OK`: Succeed
 * @retval - `ESP_ERR_INVALID_ARG`: Null arguments
 * @retval - `ESP_ERR_HTTPD_HANDLERS_FULL`: No slots left for new handler
 * @retval - `ESP_ERR_HTTPD_HANDLER_EXISTS`: Handler with same URI and method already registered
 *
 * For more details, see:
 *
 * - [ESP-IDF HTTPs Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_https_server.html)
 */
esp_err_t register_https_routes(httpd_handle_t handle);

/**
 * Registers all routes for the https server
 *
 * @param handle handle to HTTPD server instance
 * @retval - `ESP_OK`: Succeed
 * @retval - `ESP_ERR_INVALID_ARG`: Null arguments
 * @retval - `ESP_ERR_HTTPD_HANDLERS_FULL`: No slots left for new handler
 * @retval - `ESP_ERR_HTTPD_HANDLER_EXISTS`: Handler with same URI and method already registered
 *
 * For more details, see:
 *
 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
 */
esp_err_t register_http_routes(httpd_handle_t handle);

/**
 * Converts the `httpd_method_t` to the corrsponding string
 *
 * @param method The method to convert
 * @returns The string representation
 */
constexpr const char *http_method_to_str(httpd_method_t method);