#include "timeline_effect.hpp"

LedFrame TimelineEffect::get_led_data(DateTime time) {
	if (_steps.empty()) {
		return LedFrame{};
	}

	// Gesamt Dauer - wsh besser wenn von deserialize gesetzt wird
	_total_duration_ms = 0;
	for (const auto &step : _steps) {
		_total_duration_ms += step.duration_ms;
	}
	// Rechne aktuelle sekunde in ms - %gesamtloop um aktuelle position in timeline zu bekommen
	// Sollten vielleciht DateTime noch ein ms feld geben
	uint32_t current_ms = ((time.second * 1000) + 0 /*Mögliches DateTime.millisecond Feld*/) % _total_duration_ms;

	// aktiven Effekt finden
	uint32_t accumulated_ms = 0;
	for (const auto &step : _steps) {
		if (current_ms >= accumulated_ms && current_ms < (accumulated_ms + step.duration_ms)) {
			// zeit an subeffekt weiterrecihen
			return step.effect->get_led_data(time);
		}
		accumulated_ms += step.duration_ms;
	}
	// Ist timeline loop inkorrekt konfiguriert landen wir hier - eigentlich fehlerfall
	return _steps.front().effect->get_led_data(time);
}

esp_err_t TimelineEffect::serialize(const char *path) { return ESP_OK; }
esp_err_t TimelineEffect::deserialize(const char *path) {

	return EffectParser::parse_with_defaults("json_text", _path_buffer.data(), _path_buffer.size(), [](jparse_ctx_t *json_ctext) {
		// Hier nur die Feinheiten:
		// BSP:
		// json_obj_get_int(json_ctext,
		// "KEY", &this->ATTR);
	});
}

esp_err_t TimelineEffect::set_parameter(const char *name, const char *value) { return ESP_OK; }
esp_err_t TimelineEffect::set_filepath(const char *path) {
	_path = strdup(path);
	return ESP_OK;
}
const std::string TimelineEffect::get_filepath() { return _path; }

esp_err_t TimelineEffect::set_subeffect(Effect *effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}
	_subeffects.push_back(effect);
	return ESP_OK;
}

esp_err_t TimelineEffect::delete_subeffect(Effect *effect) {
	for (auto it = _subeffects.begin(); it != _subeffects.end(); ++it) {
		if (*it == effect) {
			_subeffects.erase(it);
			return ESP_OK;
		}
	}
	return ESP_ERR_NOT_FOUND;
}
std::vector<Effect *> &TimelineEffect::get_subeffects() { return _subeffects; }