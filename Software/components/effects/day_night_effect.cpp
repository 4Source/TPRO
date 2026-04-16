#include "day_night_effect.hpp"

std::unique_ptr<LedFrame> DayNightEffect::get_led_data(DateTime time) { return std::make_unique<LedFrame>(); }

esp_err_t DayNightEffect::set_filepath(const char *path) { return ESP_OK; }
std::string DayNightEffect::get_filepath() { return ""; }

esp_err_t DayNightEffect::request_api() { return ESP_OK; }

esp_err_t DayNightEffect::serialize(const char *path) { return ESP_OK; }
esp_err_t DayNightEffect::deserialize(const char *path) {
	// Default Paring macht effect_parser
	return EffectParser::parse_with_defaults("json_text", _path_buffer.data(), _path_buffer.size(), [](jparse_ctx_t *json_ctext) {
		// Hier nur die Feinheiten:
		// BSP:
		// json_obj_get_int(json_ctext,
		// "KEY", &this->ATTR);
		return ESP_OK;
	});
}

esp_err_t DayNightEffect::set_parameter(const char *name, const char *value) {
	// Not implemented for this simple test effect
	return ESP_OK;
}