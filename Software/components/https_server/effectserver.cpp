#include "effectserver.hpp"
#include "file_manager.hpp"
#include "https_server.hpp"
#include <array>
#include <esp_log.h>
#include <string>

static constexpr const char *kTag = "effectserver";

static esp_err_t path_from_uri(const char *uri, std::string &out_path) {
	if (uri == nullptr) {
		return ESP_FAIL;
	}
	const std::string uri_str(uri);
	const std::string prefix = "/effect";
	if (!uri_str.starts_with(prefix)) {
		return ESP_FAIL;
	}
	out_path = uri_str.substr(prefix.length());
	if (out_path.empty() || out_path[0] != '/') {
		out_path = "/" + out_path;
	}
	return ESP_OK;
}

esp_err_t EffectServer::handle_put_effect(httpd_req_t *req) {
	ESP_LOGD(kTag, "PUT %s", static_cast<const char *>(req->uri));

	std::string path;
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
	if (path_from_uri(req->uri, path) != ESP_OK || path.empty()) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid effect path");
		return ESP_FAIL;
	}

	// Datei leeren und neu schreiben
	if (FileManager::save_file(path, "") != ESP_OK) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot create file");
		return ESP_FAIL;
	}

	std::array<char, 512> buf{};
	int remaining = static_cast<int>(req->content_len);
	while (remaining > 0) {
		const int received = httpd_req_recv(req, buf.data(), std::min(remaining, static_cast<int>(buf.size())));
		if (received <= 0) {
			if (received == HTTPD_SOCK_ERR_TIMEOUT) {
				continue;
			}
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
			return ESP_FAIL;
		}
		if (FileManager::append_file(path, std::string(buf.data(), static_cast<size_t>(received))) != ESP_OK) {
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
			return ESP_FAIL;
		}
		remaining -= received;
	}

	if (manager != nullptr) {
		const esp_err_t reload_err = manager->reload_effect(path);
		if (reload_err != ESP_OK) {
			// Kein harter Fehler – Effekt war evtl. noch nicht geladen
			ESP_LOGW(kTag, "reload_effect(%s) returned 0x%x", path.c_str(), reload_err);
		}
	}

	httpd_resp_sendstr(req, "OK");
	return ESP_OK;
}

esp_err_t EffectServer::init(LightEffectManager &mgr) {
	manager = &mgr;
	return ESP_OK;
}
