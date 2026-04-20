#include "day_night_effect.hpp"

std::unique_ptr<LedFrame> DayNightEffect::get_led_data(DateTime time) { return std::make_unique<LedFrame>(); }

esp_err_t DayNightEffect::set_filepath(std::string path) {
	this->path_ = path;
	return ESP_OK;
}
std::string DayNightEffect::get_filepath() { return this->path_; }

esp_err_t DayNightEffect::request_api() { return ESP_OK; }

esp_err_t DayNightEffect::set_parameter(const char *name, const char *value) {
	// Not implemented for this simple test effect
	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t DayNightEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);

	if (!opt_json) {
		// Read failed
		return ESP_FAIL;
	}

	return EffectParser::parse_with_defaults(opt_json.value().c_str(), path_buffer_.data(), path_buffer_.size(), [this](jparse_ctx_t *jctx) {
		std::array<char, 32> name_buf{};
		if (json_obj_get_string(jctx, "name", name_buf.data(), name_buf.size()) == 0) {
			this->name_ = name_buf.data();
		}

		// specific parameters
		int temp_speed = 0;
		if (json_obj_get_int(jctx, "effect.parameters.default_speed", &temp_speed) == 0) {
			this->default_speed_ = static_cast<u_int32_t>(temp_speed);
		}

		int temp_brightness = 0;
		if (json_obj_get_int(jctx, "effect.parameters.default_brightness", &temp_brightness) == 0) {
			this->default_brightness_ = static_cast<u_int8_t>(temp_brightness);
		}

		return ESP_OK;
	});
}

esp_err_t DayNightEffect::serialize(std::string path) {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", this->name_.c_str());
	cJSON_AddArrayToObject(root.get(), "subeffects");

	cJSON *effect = cJSON_AddObjectToObject(root.get(), "effect");
	cJSON_AddStringToObject(effect, "type", "day_night");

	cJSON *params = cJSON_AddObjectToObject(effect, "parameters");

	cJSON_AddNumberToObject(params, "default_speed", this->default_speed_);
	cJSON_AddNumberToObject(params, "default_brightness", this->default_brightness_);

	EffectParser::cJSON_str_ptr json_string(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(path, json_string.get());
}