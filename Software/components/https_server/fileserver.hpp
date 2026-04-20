#pragma once

#include <esp_err.h>
#include <esp_https_server.h>
#include <string>

class Fileserver {
  public:
	/**
	 * GET Handler for directory listing subfolders and files in the directory on the SD Card
	 *
	 * @param req The request being responded to
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_INVALID_ARG`: Null arguments
	 * @retval - `ESP_ERR_HTTPD_RESP_SEND`: Error in raw send
	 * @retval - `ESP_ERR_HTTPD_INVALID_REQ`: Invalid request pointer
	 * @retval - `ESP_ERR_HTTPD_RESP_HDR`: Essential headers are too large for internal buffer
	 * @retval - `ESP_FAIL`: "Internal error"
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
	 */
	static esp_err_t directory_get_handler(httpd_req_t *req);

	/**
	 * DELETE Handler for deleting files on the SD Card
	 *
	 * @param req The request being responded to
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_INVALID_ARG`: Null arguments
	 * @retval - `ESP_ERR_HTTPD_RESP_SEND`: Error in raw send
	 * @retval - `ESP_ERR_HTTPD_RESP_HDR`: Essential headers are too large for internal buffer
	 * @retval - `ESP_ERR_HTTPD_INVALID_REQ`: Invalid request
	 * @retval - `ESP_FAIL`: "Internal error"
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
	 */
	static esp_err_t file_delete_handler(httpd_req_t *req);

	/**
	 * GET Handler to download files from the SD Card
	 *
	 * @param req The request being responded to
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_INVALID_ARG`: Null arguments
	 * @retval - `ESP_ERR_HTTPD_RESP_SEND`: Error in raw send
	 * @retval - `ESP_ERR_HTTPD_RESP_HDR`: Essential headers are too large for internal buffer
	 * @retval - `ESP_ERR_HTTPD_INVALID_REQ`: Invalid request
	 * @retval - `ESP_FAIL`: "Internal error"
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
	 */
	static esp_err_t file_get_handler(httpd_req_t *req);

	/**
	 * PUT Handler to upload files to the SD Card
	 *
	 * @param req The request being responded to
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_INVALID_ARG`: Null arguments
	 * @retval - `ESP_ERR_HTTPD_RESP_SEND`: Error in raw send
	 * @retval - `ESP_ERR_HTTPD_RESP_HDR`: Essential headers are too large for internal buffer
	 * @retval - `ESP_ERR_HTTPD_INVALID_REQ`: Invalid request
	 * @retval - `ESP_FAIL`: "Internal error"
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
	 */
	static esp_err_t file_put_handler(httpd_req_t *req);

  private:
	static esp_err_t get_path_from_uri(const char *uri, const std::string &prefix_uri, std::string &path);
	static constexpr const char *kTag = "fileserver";
};
