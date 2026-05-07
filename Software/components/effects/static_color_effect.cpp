#include "effect/static_color_effect.hpp"
#include <esp_err.h>
#include <esp_log.h>

esp_err_t StaticColorEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	for (auto &led_row : frame.led_data) {
		for (auto &led_cell : led_row) {
			led_cell = color;
		}
	}

	return ESP_OK;
}

esp_err_t StaticColorEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	this->path = path;
	return ESP_OK;
}

std::string StaticColorEffect::get_filepath() { return this->path; }

std::string StaticColorEffect::get_name() { return this->name; }

esp_err_t StaticColorEffect::set_parameter(const char *name, const char *value) {
	// TODO: implemented for this effect
	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t StaticColorEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);

	if (!opt_json) {
		// Read failed
		ESP_LOGW(kTag, "Read failed");
		return ESP_FAIL;
	}

	this->path = path;

	return EffectParser::parse_with_defaults(opt_json.value().c_str(), path_buffer.data(), path_buffer.size(), [this](jparse_ctx_t *jctx) {
		// version
		std::array<char, 32> version_buf{};
		if (json_obj_get_string(jctx, "version", version_buf.data(), version_buf.size()) == 0) {
			this->version = version_buf.data();
		}

		// name
		std::array<char, 32> name_buf{};
		if (json_obj_get_string(jctx, "name", name_buf.data(), name_buf.size()) == 0) {
			this->name = name_buf.data();
		}

		// type
		std::array<char, 32> type_buf{};
		if (json_obj_get_string(jctx, "type", type_buf.data(), type_buf.size()) == 0) {
			if (type_buf.data() != kType) {
				ESP_LOGW(Effect::kTag, "Type miss match durring effect parsing! expected: %s received: %s", kType, type_buf);
				return ESP_FAIL;
			}
		}

		// color Array
		int num_on = 0;
		if (json_obj_get_array(jctx, "parameters.color", &num_on) == 0 && num_on >= 3) {
			// Use temp ints for C-API
			int temp_r = 0;
			int temp_g = 0;
			int temp_b = 0;
			json_arr_get_int(jctx, 0, &temp_r);
			json_arr_get_int(jctx, 1, &temp_g);
			json_arr_get_int(jctx, 2, &temp_b);

			// cast from int
			this->color.red = static_cast<uint8_t>(temp_r);
			this->color.green = static_cast<uint8_t>(temp_g);
			this->color.blue = static_cast<uint8_t>(temp_b);
			json_obj_leave_array(jctx);
		}

		int temp_brightness = 0;
		if (json_obj_get_int(jctx, "parameters.default_brightness", &temp_brightness) == 0) {
			this->default_brightness = static_cast<uint8_t>(temp_brightness);
		}

		return ESP_OK;
	});
}

esp_err_t StaticColorEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", this->version.c_str());
	cJSON_AddStringToObject(root.get(), "name", this->name.c_str());
	cJSON_AddStringToObject(root.get(), "type", kType);
	cJSON *params = cJSON_AddObjectToObject(root.get(), "parameters");

	// Build color array
	cJSON *color_arr = cJSON_AddArrayToObject(params, "color");
	cJSON_AddItemToArray(color_arr, cJSON_CreateNumber(this->color.red));
	cJSON_AddItemToArray(color_arr, cJSON_CreateNumber(this->color.green));
	cJSON_AddItemToArray(color_arr, cJSON_CreateNumber(this->color.blue));

	cJSON_AddNumberToObject(params, "default_brightness", this->default_brightness);

	EffectParser::cJSON_str_ptr json_string(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(this->path, json_string.get());
}
