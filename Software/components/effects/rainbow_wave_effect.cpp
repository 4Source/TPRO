#include "effect/rainbow_wave_effect.hpp"
#include "color_hsv.hpp"
#include <cmath>

esp_err_t RainbowWaveEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	const uint64_t absolute_ms = (static_cast<uint64_t>(time_stamp.second) * 1000ULL) + static_cast<uint64_t>(time_stamp.millisecond);

	const float brightness = static_cast<float>(default_brightness_.value) / 255.0F;

	const size_t width = frame.led_data[0].size();

	size_t curr_y = 0;

	for (auto &row : frame.led_data) {
		size_t curr_x = 0;

		for (auto &pixel : row) {

			const size_t pixel_index = (curr_y * width) + curr_x;

			const bool in_range = (pixel_index >= led_range_start_.value) && (pixel_index <= led_range_end_.value);

			if (in_range) {
				const float spatial_offset = static_cast<float>(curr_x) * static_cast<float>(scale_.value) * 10.0F;

				const float time_offset = static_cast<float>(absolute_ms) * static_cast<float>(speed_.value) * 0.01F;

				const float hue = std::fmod(spatial_offset + time_offset, 360.0F);

				pixel = hsv_to_rgb(hue, 1.0F, brightness);
			} else {
				pixel = RGB{.red = static_cast<uint8_t>(0), .green = static_cast<uint8_t>(0), .blue = static_cast<uint8_t>(0)};
			}

			++curr_x;
		}

		++curr_y;
	}

	return ESP_OK;
}

esp_err_t RainbowWaveEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = path;
	return ESP_OK;
}

std::string RainbowWaveEffect::get_filepath() { return path_; }

std::string RainbowWaveEffect::get_name() { return this->name_; }

esp_err_t RainbowWaveEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	char *end_ptr = nullptr;
	unsigned long parsed_value = std::strtoul(value, &end_ptr, 10);

	if (end_ptr == value) {
		return ESP_FAIL;
	}

	if (strcmp(name, "default_brightness") == 0) {
		default_brightness_.value = static_cast<uint8_t>(parsed_value);
	} else if (strcmp(name, "cycle_time") == 0) {
		cycle_time_.value = static_cast<uint32_t>(parsed_value);
	} else if (strcmp(name, "led_range_start") == 0) {
		led_range_start_.value = static_cast<uint32_t>(parsed_value);
	} else if (strcmp(name, "led_range_end") == 0) {
		led_range_end_.value = static_cast<uint32_t>(parsed_value);
	} else if (strcmp(name, "speed") == 0) {
		speed_.value = static_cast<uint8_t>(parsed_value);
	} else if (strcmp(name, "scale") == 0) {
		scale_.value = static_cast<uint8_t>(parsed_value);
	} else {
		return ESP_ERR_NOT_FOUND;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t RainbowWaveEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);

	if (!opt_json) {
		return ESP_FAIL;
	}

	this->path_ = path;

	return EffectParser::parse_with_defaults(opt_json.value().c_str(), path_buffer_.data(), path_buffer_.size(), [this](jparse_ctx_t *jctx) {
		int tmp = 0;

		if (json_obj_get_int(jctx, "parameters.default_brightness", &tmp) == 0) {
			default_brightness_.value = static_cast<uint8_t>(tmp);
		}

		if (json_obj_get_int(jctx, "parameters.cycle_time", &tmp) == 0) {
			cycle_time_.value = static_cast<uint32_t>(tmp);
		}

		if (json_obj_get_int(jctx, "parameters.speed", &tmp) == 0) {
			speed_.value = static_cast<uint8_t>(tmp);
		}

		if (json_obj_get_int(jctx, "parameters.scale", &tmp) == 0) {
			scale_.value = static_cast<uint8_t>(tmp);
		}

		if (json_obj_get_array(jctx, "parameters.led_range_start", &tmp) == 0) {
			led_range_start_.value = static_cast<uint32_t>(tmp);
		}

		if (json_obj_get_array(jctx, "parameters.led_range_end", &tmp) == 0) {
			led_range_end_.value = static_cast<uint32_t>(tmp);
		}

		return ESP_OK;
	});
}

esp_err_t RainbowWaveEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *params = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(params, "default_brightness", default_brightness_.value);
	cJSON_AddNumberToObject(params, "cycle_time", cycle_time_.value);
	cJSON_AddNumberToObject(params, "speed", speed_.value);
	cJSON_AddNumberToObject(params, "scale", scale_.value);

	auto *range = cJSON_AddArrayToObject(params, "led_range");
	cJSON_AddItemToArray(range, cJSON_CreateNumber(led_range_start_.value));
	cJSON_AddItemToArray(range, cJSON_CreateNumber(led_range_end_.value));

	// ---- SCHEMA ----
	auto *schema = cJSON_AddObjectToObject(root.get(), "schema");

	auto add_schema = [&](const char *key, const ParameterSchema &schema_param) {
		auto *object = cJSON_AddObjectToObject(schema, key);
		cJSON_AddStringToObject(object, "type", schema_param.type.c_str());

		if (schema_param.type == "number" || schema_param.type == "range") {
			cJSON_AddNumberToObject(object, "min", schema_param.min);
			cJSON_AddNumberToObject(object, "max", schema_param.max);
			cJSON_AddNumberToObject(object, "step", schema_param.step);
		}
	};

	add_schema("default_brightness", default_brightness_.schema);
	add_schema("cycle_time", cycle_time_.schema);
	add_schema("speed", speed_.schema);
	add_schema("scale", scale_.schema);
	add_schema("led_range", led_range_start_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(this->path_, json.get());
}