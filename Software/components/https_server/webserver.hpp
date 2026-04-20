#pragma once

#include <esp_err.h>
#include <esp_https_server.h>

struct embed_file_data {
	const unsigned char *file_start;
	const unsigned char *file_end;
	size_t file_size;
	const char *file_content_type;
};

class Webserver {
  public:
	/**
	 * GET Handler for files which are embedded into the ESPs memory
	 *
	 * @param req The request being responded to
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_INVALID_ARG`: Null request pointer
	 * @retval - `ESP_ERR_HTTPD_INVALID_REQ`: Invalid request pointer
	 * @retval - `ESP_ERR_HTTPD_RESP_HDR`: Essential headers are too large for internal buffer
	 * @retval - `ESP_ERR_HTTPD_RESP_SEND`: Error in raw send
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
	 *
	 * - [ESP-IDF Embedding Binary
	 * Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/build-system.html#embedding-binary-data)
	 */
	static esp_err_t embedded_file_get_handler(httpd_req_t *req);

	static embed_file_data app_js_data;
	static embed_file_data index_css_data;
	static embed_file_data worldmap_svg_data;
	static embed_file_data index_html_data;
	static embed_file_data vite_svg_data;

  private:
	static constexpr const char *kTag = "webserver";
};
