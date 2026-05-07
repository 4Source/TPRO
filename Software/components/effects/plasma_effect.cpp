#include "effect/plasma_effect.hpp"
#include <cmath>

esp_err_t PlasmaEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	const uint64_t absolute_ms = (static_cast<uint64_t>(time_stamp.second) * 1000ULL) + time_stamp.millisecond;

	const float curr_time = static_cast<float>(absolute_ms) * static_cast<float>(speed_.value) * 0.002F;

	const float brightness = static_cast<float>(default_brightness_.value) / 255.0F;

	const size_t width = frame.led_data[0].size();

	for (size_t curr_y = 0; curr_y < frame.led_data.size(); ++curr_y) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
		for (size_t curr_x = 0; curr_x < frame.led_data[curr_y].size(); ++curr_x) {

			const size_t pixel_index = (curr_y * width) + curr_x;

			if (pixel_index >= led_range_start_.value && pixel_index <= led_range_end_.value) {

				const float spatial_x = static_cast<float>(curr_x) * static_cast<float>(scale_.value) * 0.3F;

				const float spatial_y = static_cast<float>(curr_y) * static_cast<float>(scale_.value) * 0.3F;

				float value = std::sin(spatial_x + curr_time) + std::sin(spatial_y + curr_time) + std::sin((spatial_x + spatial_y) + curr_time);

				value = (value + 3.0F) / 6.0F;

				const float red = static_cast<float>(color_off_.value.red) + (value * static_cast<float>(color_on_.value.red - color_off_.value.red));

				const float green =
					static_cast<float>(color_off_.value.green) + (value * static_cast<float>(color_on_.value.green - color_off_.value.green));

				const float blue =
					static_cast<float>(color_off_.value.blue) + (value * static_cast<float>(color_on_.value.blue - color_off_.value.blue));
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				frame.led_data[curr_y][curr_x] = RGB{.red = static_cast<uint8_t>(red * brightness),
													 .green = static_cast<uint8_t>(green * brightness),
													 .blue = static_cast<uint8_t>(blue * brightness)};

			} else {
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				frame.led_data[curr_y][curr_x] =
					RGB{.red = static_cast<uint8_t>(0), .green = static_cast<uint8_t>(0), .blue = static_cast<uint8_t>(0)};
			}
		}
	}

	return ESP_OK;
}

esp_err_t PlasmaEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = std::move(path);
	return ESP_OK;
}

std::string PlasmaEffect::get_filepath() { return path_; }

std::string PlasmaEffect::get_name() { return this->name_; }

esp_err_t PlasmaEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	char *end_ptr = nullptr;
	unsigned long parsed_value = std::strtoul(value, &end_ptr, 10);

	// Wenn es keine normale Dezimalzahl ist, prüfen wir, ob es ein Hex-Farbcode ist (z.B. "#FF00FF")
	if (end_ptr == value) {
		if (strcmp(name, "color_on") == 0 || strcmp(name, "color_off") == 0) {
			const char *hex_str = (*value == '#') ? std::next(value) : value;
			unsigned long hex_val = std::strtoul(hex_str, &end_ptr, 16);

			if (end_ptr != hex_str) {
				RGB color{.red = static_cast<uint8_t>((hex_val >> 16) & 0xFF),
						  .green = static_cast<uint8_t>((hex_val >> 8) & 0xFF),
						  .blue = static_cast<uint8_t>(hex_val & 0xFF)};

				if (strcmp(name, "color_on") == 0) {
					color_on_.value = color;
				} else {
					color_off_.value = color;
				}
				return ESP_OK;
			}
		}
		return ESP_FAIL;
	}

	// Zuweisung der restlichen Standard-Parameter
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
esp_err_t PlasmaEffect::deserialize(std::string path) {
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

		int num = 0;

		if (json_obj_get_array(jctx, "parameters.led_range", &num) == 0 && num >= 2) {
			int begin = 0;
			int end = 0;

			json_arr_get_int(jctx, 0, &begin);
			json_arr_get_int(jctx, 1, &end);
			json_obj_leave_array(jctx);

			led_range_start_.value = static_cast<uint32_t>(begin);
			led_range_end_.value = static_cast<uint32_t>(end);
		}

		if (json_obj_get_array(jctx, "parameters.color_on", &num) == 0 && num >= 3) {
			int red = 0;
			int green = 0;
			int blue = 0;

			json_arr_get_int(jctx, 0, &red);
			json_arr_get_int(jctx, 1, &green);
			json_arr_get_int(jctx, 2, &blue);
			json_obj_leave_array(jctx);

			color_on_.value = RGB{.red = static_cast<uint8_t>(red), .green = static_cast<uint8_t>(green), .blue = static_cast<uint8_t>(blue)};
		}

		if (json_obj_get_array(jctx, "parameters.color_off", &num) == 0 && num >= 3) {
			int red = 0;
			int green = 0;
			int blue = 0;

			json_arr_get_int(jctx, 0, &red);
			json_arr_get_int(jctx, 1, &green);
			json_arr_get_int(jctx, 2, &blue);
			json_obj_leave_array(jctx);

			color_off_.value = RGB{.red = static_cast<uint8_t>(red), .green = static_cast<uint8_t>(green), .blue = static_cast<uint8_t>(blue)};
		}

		return ESP_OK;
	});
}

esp_err_t PlasmaEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *params = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(params, "default_brightness", default_brightness_.value);
	cJSON_AddNumberToObject(params, "cycle_time", cycle_time_.value);
	cJSON_AddNumberToObject(params, "speed", speed_.value);
	cJSON_AddNumberToObject(params, "scale", scale_.value);

	auto *range = cJSON_AddArrayToObject(params, "led_range");
	cJSON_AddItemToArray(range, cJSON_CreateNumber(led_range_start_.value));
	cJSON_AddItemToArray(range, cJSON_CreateNumber(led_range_end_.value));

	// NOLINTNEXTLINE[readability-identifier-length]
	auto *on = cJSON_AddArrayToObject(params, "color_on");
	cJSON_AddItemToArray(on, cJSON_CreateNumber(color_on_.value.red));
	cJSON_AddItemToArray(on, cJSON_CreateNumber(color_on_.value.green));
	cJSON_AddItemToArray(on, cJSON_CreateNumber(color_on_.value.blue));

	auto *off = cJSON_AddArrayToObject(params, "color_off");
	cJSON_AddItemToArray(off, cJSON_CreateNumber(color_off_.value.red));
	cJSON_AddItemToArray(off, cJSON_CreateNumber(color_off_.value.green));
	cJSON_AddItemToArray(off, cJSON_CreateNumber(color_off_.value.blue));

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
	add_schema("color_on", color_on_.schema);
	add_schema("color_off", color_off_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}