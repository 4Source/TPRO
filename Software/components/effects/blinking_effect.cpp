#include "blinking_effect.hpp"
#include <esp_err.h>

std::unique_ptr<LedFrame> BlinkingEffect::get_led_data(DateTime time_stamp) {
	auto frame = std::make_unique<LedFrame>();

	// Implements a simple blinking logic: 1 second ON, 1 second OFF
	bool is_on = (time_stamp.second % 2) == 0;
	RGB current_color = is_on ? color_on_ : color_off_;

	for (auto &led_row : frame->led_data) {
		for (auto &led_pos : led_row) {
			led_pos = current_color;
		}
	}

	return frame;
}

esp_err_t BlinkingEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = path;
	return ESP_OK;
}

std::string BlinkingEffect::get_filepath() { return path_; }

esp_err_t BlinkingEffect::set_parameter(const char *name, const char *value) {
	// Not implemented for this simple test effect
	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t BlinkingEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);

	if (!opt_json) {
		// Read failed
		return ESP_FAIL;
	}

	return EffectParser::parse_with_defaults(opt_json.value().c_str(), path_buffer_.data(), path_buffer_.size(), [this](jparse_ctx_t *jctx) {
		// name
		std::array<char, 32> name_buf{};
		if (json_obj_get_string(jctx, "name", name_buf.data(), name_buf.size()) == 0) {
			this->name_ = name_buf.data();
		}

		// color_on Array
		int num_on = 0;
		if (json_obj_get_array(jctx, "effect.parameters.color_on", &num_on) == 0 && num_on >= 3) {
			// Use temp ints for C-API
			int temp_r = 0;
			int temp_g = 0;
			int temp_b = 0;
			json_arr_get_int(jctx, 0, &temp_r);
			json_arr_get_int(jctx, 1, &temp_g);
			json_arr_get_int(jctx, 2, &temp_b);

			// cast from int
			this->color_on_.red = static_cast<u_int8_t>(temp_r);
			this->color_on_.green = static_cast<u_int8_t>(temp_g);
			this->color_on_.blue = static_cast<u_int8_t>(temp_b);
			json_obj_leave_array(jctx);
		}

		// color_off Array
		int num_off = 0;
		if (json_obj_get_array(jctx, "effect.parameters.color_off", &num_off) == 0 && num_off >= 3) {
			// Use temp ints for C-API
			int temp_r = 0;
			int temp_g = 0;
			int temp_b = 0;
			json_arr_get_int(jctx, 0, &temp_r);
			json_arr_get_int(jctx, 1, &temp_g);
			json_arr_get_int(jctx, 2, &temp_b);

			this->color_off_.red = static_cast<u_int8_t>(temp_r);
			this->color_off_.green = static_cast<u_int8_t>(temp_g);
			this->color_off_.blue = static_cast<u_int8_t>(temp_b);
			json_obj_leave_array(jctx);
		}

		// standard
		int temp_cycle = 0;
		if (json_obj_get_int(jctx, "effect.parameters.cycle_time", &temp_cycle) == 0) {
			this->cycle_time_ = static_cast<u_int32_t>(temp_cycle);
		}

		int temp_brightness = 0;
		if (json_obj_get_int(jctx, "effect.parameters.default_brightness", &temp_brightness) == 0) {
			this->default_brightness_ = static_cast<u_int8_t>(temp_brightness);
		}

		// led range
		int num_elems = 0;
		if (json_obj_get_array(jctx, "effect.parameters.led_range", &num_elems) == 0 && num_elems >= 2) {
			int temp_start = 0;
			int temp_end = 0;
			json_arr_get_int(jctx, 0, &temp_start);
			json_arr_get_int(jctx, 1, &temp_end);

			this->led_range_start_ = static_cast<u_int32_t>(temp_start);
			this->led_range_end_ = static_cast<u_int32_t>(temp_end);
			json_obj_leave_array(jctx);
		}

		return ESP_OK;
	});
}

esp_err_t BlinkingEffect::serialize(std::string path) {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", this->name_.c_str());
	cJSON_AddArrayToObject(root.get(), "subeffects");

	cJSON *effect = cJSON_AddObjectToObject(root.get(), "effect");
	cJSON_AddStringToObject(effect, "type", "blink");

	cJSON *params = cJSON_AddObjectToObject(effect, "parameters");

	cJSON_AddNumberToObject(params, "cycle_time", this->cycle_time_);
	cJSON_AddNumberToObject(params, "default_brightness", this->default_brightness_);

	// Build _color_on array
	cJSON *color_on_arr = cJSON_AddArrayToObject(params, "color_on");
	cJSON_AddItemToArray(color_on_arr, cJSON_CreateNumber(this->color_on_.red));
	cJSON_AddItemToArray(color_on_arr, cJSON_CreateNumber(this->color_on_.green));
	cJSON_AddItemToArray(color_on_arr, cJSON_CreateNumber(this->color_on_.blue));

	// Build _color_off array
	cJSON *color_off_arr = cJSON_AddArrayToObject(params, "color_off");
	cJSON_AddItemToArray(color_off_arr, cJSON_CreateNumber(this->color_off_.red));
	cJSON_AddItemToArray(color_off_arr, cJSON_CreateNumber(this->color_off_.green));
	cJSON_AddItemToArray(color_off_arr, cJSON_CreateNumber(this->color_off_.blue));

	// Build _led_range array
	cJSON *range = cJSON_AddArrayToObject(params, "led_range");
	cJSON_AddItemToArray(range, cJSON_CreateNumber(this->led_range_start_));
	cJSON_AddItemToArray(range, cJSON_CreateNumber(this->led_range_end_));

	EffectParser::cJSON_str_ptr json_string(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(path, json_string.get());
}
