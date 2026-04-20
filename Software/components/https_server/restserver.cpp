#include "restserver.hpp"
#include "routes.hpp"
#include <esp_log.h>

static constexpr const char *kTag = "restserver";

esp_err_t RestServer::handle_get_config(httpd_req_t *req) {

	// get query string
	size_t query_string_length = httpd_req_get_url_query_len(req);
	if (query_string_length < 1) {
		ESP_LOGW(kTag, "No query found");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query found");
		return ESP_ERR_INVALID_SIZE;
	};
	std::string query_string;
	query_string.resize(query_string_length);
	httpd_req_get_url_query_str(req, query_string.data(), query_string_length + 1);

	// perform GET
	if (my_config_manager != nullptr && !query_string.empty()) {
		std::string value = my_config_manager->get_config(query_string);

		if (value.empty()) {
			ESP_LOGE(kTag, "Requested route (handle_get_config): %s=", query_string.c_str());
			httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid key");
			return ESP_FAIL;
		}

		ESP_LOGD(kTag, "Requested route (handle_get_config): %s=%s", query_string.c_str(), value.c_str());

		httpd_resp_sendstr(req, value.c_str());
	} else if (query_string.empty()) {
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid Request");
		return ESP_FAIL;
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid Server state");
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t RestServer::handle_put_config(httpd_req_t *req) {

	// get query string
	size_t query_string_length = httpd_req_get_url_query_len(req);
	if (query_string_length < 1) {
		ESP_LOGW(kTag, "No query found");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query found");
		return ESP_ERR_INVALID_SIZE;
	};
	std::string query_string;
	query_string.resize(query_string_length);
	httpd_req_get_url_query_str(req, query_string.data(), query_string_length + 1);

	// perform PUT
	if (my_config_manager != nullptr && !query_string.empty() && query_string.contains("=")) {
		size_t equal_pos = query_string.find('=');
		std::string key = query_string.substr(0, equal_pos);
		std::string value = query_string.substr(equal_pos + 1);

		ESP_LOGD(kTag, "Requested route (handle_put_config): %s=%s", key.c_str(), value.c_str());

		if (my_config_manager->set_config(key, value) != ESP_OK) {
			httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid key value pair");
			return ESP_FAIL;
		}
	} else if (query_string.empty() || !query_string.contains("=")) {
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid Request");
		return ESP_FAIL;
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid Server state");
		return ESP_FAIL;
	}

	httpd_resp_send(req, nullptr, 0);
	return ESP_OK;
}

esp_err_t RestServer::handle_delete_config(httpd_req_t *req) {

	// get query string
	size_t query_string_length = httpd_req_get_url_query_len(req);
	if (query_string_length < 1) {
		ESP_LOGW(kTag, "No query found");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query found");
		return ESP_ERR_INVALID_SIZE;
	};
	std::string query_string;
	query_string.resize(query_string_length);
	httpd_req_get_url_query_str(req, query_string.data(), query_string_length + 1);

	// perform DELETE
	if (my_config_manager != nullptr && !query_string.empty()) {
		if (my_config_manager->set_to_default(query_string) != ESP_OK) {
			httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid key");
			return ESP_FAIL;
		}
		std::string value = my_config_manager->get_config(query_string);
		ESP_LOGD(kTag, "Requested route (handle_delete_config): %s=%s", query_string.c_str(), value.c_str());

		httpd_resp_sendstr(req, value.c_str());
	} else if (query_string.empty()) {
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid Request");
		return ESP_FAIL;
	} else {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid Server state");
		return ESP_FAIL;
	}

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