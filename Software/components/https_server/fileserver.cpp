#include "fileserver.hpp"
#include "file_manager.hpp"
#include "https_server.hpp"
#include <cJSON.h>
#include <esp_check.h>
#include <esp_log.h>

esp_err_t Fileserver::directory_get_handler(httpd_req_t *req) {
	ESP_LOGD(kTag, "Requested route (directory_get_handler): %s %s", HttpsServer::http_method_to_str(static_cast<httpd_method_t>(req->method)),
			 static_cast<const char *>(req->uri));

	const std::string files_prefix = "/directory/";
	std::string path;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
	if (get_path_from_uri(req->uri, files_prefix, path) != ESP_OK) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
		std::string uri_str(req->uri);
		if ((uri_str.length() == files_prefix.length() && uri_str == files_prefix) ||
			(uri_str.length() == files_prefix.length() - 1 && uri_str == files_prefix.substr(0, files_prefix.length() - 1))) {
			// Specifically allow '/directory/' and '/directory'
			path = "/";
		} else {
			ESP_LOGE(kTag, "Failed to read out path from 'list' uri");
			ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read out path from uri"), kTag,
								"Failed to send response");
			return ESP_FAIL;
		}
	}

	if (path.empty()) {
		ESP_LOGE(kTag, "Empty path received");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty path received"), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	esp_err_t err = FileManager::is_directory(path);
	if (err == ESP_FAIL) {
		ESP_LOGE(kTag, "Invalid path received");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path received"), kTag, "Failed to send response");
		return ESP_FAIL;
	}
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to read path");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read path"), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	// Create JSON from files
	std::vector<std::string> files = FileManager::list_directory(path);
	cJSON *json_root = cJSON_CreateArray();

	for (auto &&entry : files) {
		ESP_LOGW(kTag, "%s", entry.c_str());
		cJSON_AddItemToArray(json_root, cJSON_CreateString(entry.data()));
	}

	char *json_string = cJSON_Print(json_root);
	cJSON_Delete(json_root);

	// Set the content type of the response to the type of the file
	err = httpd_resp_set_type(req, "application/json");
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to set the content type for the response");
		return err;
	}

	// Send the file as response
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	err = httpd_resp_sendstr(req, json_string);
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to set the content for the response");
		return err;
	}

	return ESP_OK;
}

esp_err_t Fileserver::file_delete_handler(httpd_req_t *req) {
	ESP_LOGD(kTag, "Requested route (file_delete_handler): %s %s", HttpsServer::http_method_to_str(static_cast<httpd_method_t>(req->method)),
			 static_cast<const char *>(req->uri));

	const std::string delete_prefix = "/file/";
	std::string path;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
	if (get_path_from_uri(req->uri, delete_prefix, path) != ESP_OK) {
		ESP_LOGE(kTag, "Failed to read out path from 'delete' uri");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read out path from uri"), kTag,
							"Failed to send response");
		return ESP_FAIL;
	}

	if (path.empty()) {
		ESP_LOGE(kTag, "Empty path received");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty path received"), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	esp_err_t err = FileManager::is_file(path);
	if (err == ESP_FAIL) {
		ESP_LOGE(kTag, "Invalid file path received");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid file path received"), kTag, "Failed to send response");
		return ESP_FAIL;
	}
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to read path");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read path"), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	if (FileManager::delete_file(path) != ESP_OK) {
		std::string msg = "Failed to delete: " + path;
		ESP_LOGE(kTag, "%s", msg.c_str());
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str()), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	httpd_resp_sendstr(req, "File deleted successfully");

	return ESP_OK;
}

esp_err_t Fileserver::file_get_handler(httpd_req_t *req) {
	ESP_LOGD(kTag, "Requested route (file_get_handler): %s %s", HttpsServer::http_method_to_str(static_cast<httpd_method_t>(req->method)),
			 static_cast<const char *>(req->uri));

	const std::string download_prefix = "/file/";
	std::string path;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
	if (get_path_from_uri(req->uri, download_prefix, path) != ESP_OK) {
		ESP_LOGE(kTag, "Failed to read out path from 'download' uri");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read out path from uri"), kTag,
							"Failed to send response");
		return ESP_FAIL;
	}

	if (path.empty()) {
		ESP_LOGE(kTag, "Empty path received");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty path received"), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	esp_err_t err = FileManager::is_file(path);
	if (err == ESP_FAIL) {
		ESP_LOGE(kTag, "Invalid file path received");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid file path received"), kTag, "Failed to send response");
		return ESP_FAIL;
	}
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to read path");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read path"), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	// Set the content type of the response to the type of the file
	err = httpd_resp_set_type(req, "application/json");
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to set the content type for the response");
		return err;
	}

	err = FileManager::read_file_chunked<512>(path, [&req](const char *data, int len) -> esp_err_t {
		ESP_LOGD(kTag, "File content:\n%s", data);

		// Send the file as response
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		esp_err_t err = httpd_resp_send_chunk(req, data, len);
		if (err != ESP_OK) {
			ESP_LOGE(kTag, "Failed to set the content for the response");
			return err;
		}

		return ESP_OK;
	});

	if (err != ESP_OK) {
		std::string msg = "Failed to download: " + path;
		ESP_LOGE(kTag, "%s", msg.c_str());
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg.c_str()), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	err = httpd_resp_send_chunk(req, nullptr, 0);
	if (err != ESP_OK) {
		std::string msg = "Failed to download: " + path;
		ESP_LOGE(kTag, "%s", msg.c_str());
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg.c_str()), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t Fileserver::file_put_handler(httpd_req_t *req) {
	ESP_LOGD(kTag, "Requested route (file_put_handler): %s %s", HttpsServer::http_method_to_str(static_cast<httpd_method_t>(req->method)),
			 static_cast<const char *>(req->uri));

	const std::string upload_prefix = "/file/";
	std::string path;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
	if (get_path_from_uri(req->uri, upload_prefix, path) != ESP_OK) {
		ESP_LOGE(kTag, "Failed to read out path from 'upload' uri");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read out path from uri"), kTag,
							"Failed to send response");
		return ESP_FAIL;
	}

	if (path.empty()) {
		ESP_LOGE(kTag, "Empty path received");
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty path received"), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	// Create/Overwrite file with no content
	if (FileManager::save_file(path, "") != ESP_OK) {
		std::string msg = "Failed to create file: " + path;
		ESP_LOGE(kTag, "%s", msg.c_str());
		ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str()), kTag, "Failed to send response");
		return ESP_FAIL;
	}

	// Readout content from request
	std::array<char, 512> scratch_buffer;
	int remaining = static_cast<int>(req->content_len);
	int received = 0;
	while (remaining > 0) {
		// Receive the file part by part into a buffer
		received = httpd_req_recv(req, scratch_buffer.data(), std::min(remaining, static_cast<int>(scratch_buffer.size())));
		if (received <= 0) {
			if (received == HTTPD_SOCK_ERR_TIMEOUT) {
				// Retry if timeout occurred
				continue;
			}

			ESP_LOGE(kTag, "File reception failed!");
			// Respond with 500 Internal Server Error
			ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file"), kTag, "Failed to send response");
			return ESP_FAIL;
		}

		const std::string content(scratch_buffer.data(), received);
		ESP_LOGD(kTag, "Received %d content:\n%s", received, content.c_str());
		// Write buffer content to file on storage
		if (FileManager::append_file(path, content) != ESP_OK) {
			std::string msg = "Failed to write file: " + path;
			ESP_LOGE(kTag, "%s", msg.c_str());
			ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str()), kTag, "Failed to send response");
			return ESP_FAIL;
		}

		/* Keep track of remaining size of the file left to be uploaded */
		remaining -= received;
	}

	ESP_RETURN_ON_ERROR(httpd_resp_set_status(req, "201 Created"), kTag, "Failed to set response status");
	ESP_RETURN_ON_ERROR(httpd_resp_sendstr(req, "File uploaded successfully"), kTag, "Failed to send response");

	return ESP_OK;
}

esp_err_t Fileserver::get_path_from_uri(const char *uri, const std::string &prefix_uri, std::string &path) {
	if (uri == nullptr) {
		path.clear();
		return ESP_FAIL;
	}

	std::string uri_str(uri);

	if (uri_str.length() < prefix_uri.length()) {
		path.clear();
		ESP_LOGW(kTag, "URI is shorter than expected prefix");
		return ESP_FAIL;
	}

	if (!uri_str.starts_with(prefix_uri)) {
		ESP_LOGW(kTag, "URI does not start with prefix");
		path.clear();
		return ESP_FAIL;
	}

	// Extract everything AFTER the prefix
	path = uri_str.substr(prefix_uri.length());

	if (path.empty()) {
		ESP_LOGW(kTag, "URI is empty after removeing prefix");
		path.clear();
		return ESP_FAIL;
	}

	// ensure leading slash
	if (path[0] != '/') {
		path = "/" + path;
	}

	return ESP_OK;
}
