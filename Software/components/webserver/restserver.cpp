#include "restserver.hpp"
#include "routes.hpp"
#include <esp_log.h>

static constexpr const char *kTag = "restserver";

esp_err_t RestServer::handle_get_config(httpd_req_t *req) {

	// get query string
	size_t len = httpd_req_get_url_query_len(req);
	if (len < 1) {
		ESP_LOGI(kTag, "No query found");
		return ESP_ERR_INVALID_SIZE;
	};
	std::string val;
	val.resize(len);
	httpd_req_get_url_query_str(req, val.data(), len + 1);
	ESP_LOGI(kTag, "get %s", val.c_str());

	// perform GET
	if (my_config_manager != nullptr) {
		std::string result = my_config_manager->get_config(val);

		ESP_LOGI(kTag, "value: %s", result.c_str());

		httpd_resp_sendstr(req, result.c_str());
	}
	return ESP_OK;
}
esp_err_t RestServer::handle_put_config(httpd_req_t *req) {

	// get query string
	size_t len = httpd_req_get_url_query_len(req);
	if (len < 1) {
		ESP_LOGI(kTag, "No query found");
		return ESP_ERR_INVALID_SIZE;
	};
	std::string val;
	val.resize(len);
	httpd_req_get_url_query_str(req, val.data(), len + 1);
	ESP_LOGI(kTag, "put %s", val.c_str());

	// perform PUT
	if (my_config_manager != nullptr && !val.empty() && val.contains("=")) {
		size_t equal_pos = val.find('=');
		std::string key = val.substr(0, equal_pos);
		std::string value = val.substr(equal_pos + 1);
		ESP_LOGI(kTag, "key: %s, value: %s", key.c_str(), value.c_str());
		my_config_manager->set_config(key, value);
	}
	httpd_resp_send(req, nullptr, 0);
	return ESP_OK;
}

esp_err_t RestServer::handle_delete_config(httpd_req_t *req) {

	// get query string
	size_t len = httpd_req_get_url_query_len(req);
	if (len < 1) {
		ESP_LOGI(kTag, "No query found");
		return ESP_ERR_INVALID_SIZE;
	};
	std::string val;
	val.resize(len);
	httpd_req_get_url_query_str(req, val.data(), len + 1);
	ESP_LOGI(kTag, "delete %s", val.c_str());

	// perform DELETE
	if (my_config_manager != nullptr && !val.empty()) {
		my_config_manager->set_to_default(val);
		ESP_LOGI(kTag, "set to default value: %s", my_config_manager->get_config(val).c_str());
	}
	httpd_resp_send(req, nullptr, 0);
	return ESP_OK;
}

esp_err_t RestServer::init(ConfigManager &config_manager, httpd_handle_t server) {
	my_config_manager = &config_manager;
	httpd_server = server;
	if (httpd_server == nullptr) {
		ESP_LOGE(kTag, "Failed to start REST, server handle is null");
		return ESP_ERR_INVALID_STATE;
	}

	return ESP_OK;
}