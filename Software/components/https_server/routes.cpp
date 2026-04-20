#include "routes.hpp"
#include "fileserver.hpp"
#include "https_server.hpp"
#include "restserver.hpp"
#include "webserver.hpp"
#include "websocket_server.hpp"
#include <array>
#include <esp_log.h>
#include <string>

static constexpr const char *kTag = "routes";

/**
 * Registers a route and the handler for it
 *
 * @param handle handle to HTTPD server instance
 * @param uri_handler pointer to handler that needs to be registered
 * @retval - `ESP_OK`: Succeed
 * @retval - `ESP_ERR_INVALID_ARG`: Null arguments
 * @retval - `ESP_ERR_HTTPD_HANDLERS_FULL`: No slots left for new handler
 * @retval - `ESP_ERR_HTTPD_HANDLER_EXISTS`: Handler with same URI and method already registered
 *
 * For more details, see:
 *
 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
 */
static esp_err_t register_route(httpd_handle_t handle, const httpd_uri_t *uri_handler) {
	esp_err_t ret = httpd_register_uri_handler(handle, uri_handler);
	if (ret == ESP_OK) {
		ESP_LOGI(kTag, "\tRegistered route: %s %s", HttpsServer::http_method_to_str(uri_handler->method), uri_handler->uri);
	} else {
		ESP_LOGE(kTag, "\tFailed to registered route: %s %s", HttpsServer::http_method_to_str(uri_handler->method), uri_handler->uri);
	}
	return ret;
}

// NOLINTBEGIN(cppcoreguidelines-interfaces-global-init)
/**
 * Configurations for routes
 *
 * First register the fixed assets routes and than register all remaining routes to point to the index.html and let it handle the rest. Routing is
 * than done by the browser including error pages.
 */
static const std::array kHttpsRoutes{
	httpd_uri_t{.uri = "/ws",
				.method = HTTP_GET,
				.handler = WebsocketServer::ws_handler,
				.user_ctx = nullptr,
				.is_websocket = true,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/assets/app.js",
				.method = HTTP_GET,
				.handler = Webserver::embedded_file_get_handler,
				.user_ctx = &Webserver::app_js_data,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/assets/index.css",
				.method = HTTP_GET,
				.handler = Webserver::embedded_file_get_handler,
				.user_ctx = &Webserver::index_css_data,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/assets/worldmap.svg",
				.method = HTTP_GET,
				.handler = Webserver::embedded_file_get_handler,
				.user_ctx = &Webserver::worldmap_svg_data,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/config",
				.method = HTTP_GET,
				.handler = RestServer::handle_get_config,
				.user_ctx = nullptr,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/config",
				.method = HTTP_PUT,
				.handler = RestServer::handle_put_config,
				.user_ctx = nullptr,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/config",
				.method = HTTP_DELETE,
				.handler = RestServer::handle_delete_config,
				.user_ctx = nullptr,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/directory",
				.method = HTTP_GET,
				.handler = Fileserver::directory_get_handler,
				.user_ctx = nullptr,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/directory/*",
				.method = HTTP_GET,
				.handler = Fileserver::directory_get_handler,
				.user_ctx = nullptr,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/file/*",
				.method = HTTP_DELETE,
				.handler = Fileserver::file_delete_handler,
				.user_ctx = nullptr,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/file/*",
				.method = HTTP_GET,
				.handler = Fileserver::file_get_handler,
				.user_ctx = nullptr,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/file/*",
				.method = HTTP_PUT,
				.handler = Fileserver::file_put_handler,
				.user_ctx = nullptr,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/vite.svg",
				.method = HTTP_GET,
				.handler = Webserver::embedded_file_get_handler,
				.user_ctx = &Webserver::vite_svg_data,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
	httpd_uri_t{.uri = "/*",
				.method = HTTP_GET,
				.handler = Webserver::embedded_file_get_handler,
				.user_ctx = &Webserver::index_html_data,
				.is_websocket = false,
				.handle_ws_control_frames = false,
				.supported_subprotocol = nullptr},
};
// NOLINTEND(cppcoreguidelines-interfaces-global-init)

esp_err_t register_https_routes(httpd_handle_t handle) {
	ESP_LOGI(kTag, "Register https routes:");
	for (const auto &route : kHttpsRoutes) {
		esp_err_t err = register_route(handle, &route);
		if (err != ESP_OK) {
			ESP_LOGE(kTag, "Failed to register route");
			return err;
		}
	}
	return ESP_OK;
}

uint16_t get_number_of_https_routes() { return kHttpsRoutes.size(); }
