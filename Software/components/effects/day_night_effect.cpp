#include "day_night_effect.hpp"

LedFrame DayNightEffect::get_led_data(DateTime time) { return LedFrame{}; }

esp_err_t DayNightEffect::set_subeffect(Effect *effect) { return ESP_OK; }
std::vector<Effect *> &DayNightEffect::get_subeffects() { return _subeffects; }

esp_err_t DayNightEffect::set_filepath(const char *path) { return ESP_OK; }
const char *DayNightEffect::get_filepath() { return ""; }

esp_err_t DayNightEffect::request_api() { return ESP_OK; }

esp_err_t DayNightEffect::serialize() { return ESP_OK; }
esp_err_t DayNightEffect::deserialize(const char *json_text) {
	// Default Paring macht effect_parser
	return EffectParser::parse_with_defaults(json_text, _path, sizeof(_path), [](jparse_ctx_t *json_ctext) {
		// Hier nur die Feinheiten:
		// BSP:
		// json_obj_get_int(json_ctext,
		// "KEY", &this->ATTR);
	});
}