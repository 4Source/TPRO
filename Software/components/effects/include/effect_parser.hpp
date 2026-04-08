#pragma once
// include json parser
// https://github.com/espressif/json_parser/blob/master/include/json_parser.h
// Für JSON Serialisierung/Deserialisierung der Effekte
#include <json_parser.h>
#include <cstring>
#include <esp_err.h>

struct EffectParser {
  // Diese Funktion übernimmt das Standard-Parsing (für sämtliche Effekte
  // identisch) und ruft dann die spezifische Logik auf
  template <typename F>
  static esp_err_t parse_with_defaults(
      const char *json_text, char *path_out, size_t path_len,
      F &&specific_logic /*&& - müssen dann nur ein lambda übergeben*/) {
    jparse_ctx_t json_ctext;
    if (json_parse_start(&json_ctext, json_text, strlen(json_text)) !=
        OS_SUCCESS) {
      return ESP_FAIL;
    }

    // Standrd-parser
    json_obj_get_string(&json_ctext, "path", path_out, path_len);

    // Spezial-Parser
    specific_logic(&json_ctext);

    json_parse_end(&json_ctext);
    return ESP_OK;
  }
};