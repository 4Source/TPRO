#pragma once
#include "config_manager.hpp"
#include <esp_err.h>
#include <esp_https_server.h>

class RestServer {

	// helpful link https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html
  public:
	inline static httpd_handle_t httpd_server;
	inline static ConfigManager *my_config_manager = nullptr;
	RestServer(httpd_handle_t server, ConfigManager config_manager);

	// GET returns value.
	// example /config?speed
	static esp_err_t handle_get_config(httpd_req_t *req);

	// PUT sets value.
	// example /config?speed=5
	static esp_err_t handle_put_config(httpd_req_t *req);

	// DELETE sets value to default.
	// example /config?speed
	static esp_err_t handle_delete_config(httpd_req_t *req);

	// Initializes REST using server and config_manager
	static esp_err_t init(ConfigManager &config_manager, httpd_handle_t server);
};