#pragma once
// include json parser
// https://github.com/espressif/json_parser/blob/master/include/json_parser.h
// Für JSON Serialisierung/Deserialisierung der Effekte
#include "cJSON.h"
#include <cstring>
#include <esp_err.h>
#include <json_parser.h>
#include <memory>

struct EffectParser {
	// RAII Wrappers for cJSON
	using cJSON_ptr = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;
	using cJSON_str_ptr = std::unique_ptr<char, decltype(&free)>;

	// Diese Funktion übernimmt das Standard-Parsing (für sämtliche Effekte
	// identisch) und ruft dann die spezifische Logik auf
	template <typename F>
	static esp_err_t parse_with_defaults(const char *json_text, char *path_out, size_t path_len,
										 F &&specific_logic /* - müssen dann nur ein lambda übergeben*/) {
		jparse_ctx_t json_ctext;
		if (json_parse_start(&json_ctext, json_text, static_cast<int>(strlen(json_text))) != OS_SUCCESS) {
			return ESP_FAIL;
		}

		// Standrd-parser
		if (json_obj_get_string(&json_ctext, "path", path_out, static_cast<int>(path_len)) != OS_SUCCESS) {
			if (path_len > 0) {
				path_out[0] = '\0';
			}
		}

		// Spezial-Parser
		esp_err_t ret = ESP_FAIL;

		ret = std::forward<F>(specific_logic)(&json_ctext);

		json_parse_end(&json_ctext);
		return ret;
	}
};