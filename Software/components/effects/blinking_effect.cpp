#include "effect/blinking_effect.hpp"
#include <esp_err.h>

esp_err_t BlinkingEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	// 1. Berechnung des Status
	const uint32_t cycle = std::max<uint32_t>(1, cycle_time_.value);
	const uint32_t total_ms = (time_stamp.second * 1000) + time_stamp.millisecond;
	const bool is_on = (total_ms / cycle) % 2 == 0;

	// 2. Farbberechnung (Optimiert: Nur einmal rechnen)
	const RGB &base_color = is_on ? color_on_.value : color_off_.value;
	const float brightness = static_cast<float>(default_brightness_.value) / 255.0F;

	RGB current_color{.red = static_cast<uint8_t>(static_cast<float>(base_color.red) * brightness),
					  .green = static_cast<uint8_t>(static_cast<float>(base_color.green) * brightness),
					  .blue = static_cast<uint8_t>(static_cast<float>(base_color.blue) * brightness)};

	const uint32_t start = led_range_.value[0];
	const uint32_t end = led_range_.value[1];

	// 3. LED Daten füllen
	uint32_t index = 0;
	for (auto &led_row : frame.led_data) {
		for (auto &led_pos : led_row) {
			// Korrigierte Logik: Nur innerhalb des Range die Farbe setzen, sonst Schwarz
			if (index >= start && index <= end) {
				led_pos = current_color;
			} else {
				led_pos = RGB{.red = 0, .green = 0, .blue = 0};
			}
			index++;
		}
	}
	return ESP_OK;
}

esp_err_t BlinkingEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	this->path = path;
	return ESP_OK;
}

std::string BlinkingEffect::get_filepath() { return this->path; }

std::string BlinkingEffect::get_name() { return this->name_; }

esp_err_t BlinkingEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	if (strcmp(name, "cycle_time") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		cycle_time_.value = val;
	} else if (strcmp(name, "default_brightness") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}
		default_brightness_.value = static_cast<uint8_t>(val);
	} else if (strcmp(name, "led_range") == 0) {
		uint32_t start = 0U;
		uint32_t end = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu32 ",%" SCNu32, &start, &end) != 2) {
			return ESP_ERR_INVALID_ARG;
		}
		led_range_.value = {start, end};
	} else if (strcmp(name, "color_on") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_on_.value = RGB{.red = red, .green = green, .blue = blue};
	} else if (strcmp(name, "color_off") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_off_.value = RGB{.red = red, .green = green, .blue = blue};
	} else {
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t BlinkingEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);

	if (!opt_json) {
		return ESP_FAIL;
	}

	this->path = path;

	return EffectParser::parse_with_defaults(opt_json->c_str(), path_buffer_.data(), path_buffer_.size(), [this](jparse_ctx_t *jctx) {
		std::array<char, 64> buf{};

		// ---- name ----
		if (json_obj_get_string(jctx, "name", buf.data(), buf.size()) == 0) {
			name_ = buf.data();
		}

		// ---- simple numbers ----
		int tmp = 0;

		if (json_obj_get_int(jctx, "parameters.cycle_time", &tmp) == 0) {
			cycle_time_.value = tmp;
		}

		if (json_obj_get_int(jctx, "parameters.default_brightness", &tmp) == 0) {
			default_brightness_.value = tmp;
		}

		// ---- RGB helper ----
		auto read_rgb = [&](const char *key, RGB &out) {
			int red = 0;
			int green = 0;
			int blue = 0;

			if (json_obj_get_array(jctx, key, nullptr) == 0) {
				json_arr_get_int(jctx, 0, &red);
				json_arr_get_int(jctx, 1, &green);
				json_arr_get_int(jctx, 2, &blue);
				json_obj_leave_array(jctx);

				out = RGB{.red = static_cast<uint8_t>(red), .green = static_cast<uint8_t>(green), .blue = static_cast<uint8_t>(blue)};
			}
		};

		read_rgb("parameters.color_on", color_on_.value);
		read_rgb("parameters.color_off", color_off_.value);

		// ---- range ----
		int start = 0;
		int end = 0;

		if (json_obj_get_array(jctx, "parameters.led_range", nullptr) == 0) {
			json_arr_get_int(jctx, 0, &start);
			json_arr_get_int(jctx, 1, &end);
			json_obj_leave_array(jctx);

			led_range_.value = {static_cast<uint32_t>(start), static_cast<uint32_t>(end)};
		}

		return ESP_OK;
	});
}

esp_err_t BlinkingEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());

	cJSON_AddStringToObject(root.get(), "type", "blink");

	auto *params = cJSON_AddObjectToObject(root.get(), "parameters");

	// ---- VALUES ----
	cJSON_AddNumberToObject(params, "cycle_time", cycle_time_.value);
	cJSON_AddNumberToObject(params, "default_brightness", default_brightness_.value);

	auto add_rgb = [&](const char *key, const RGB &color) {
		auto *arr = cJSON_AddArrayToObject(params, key);
		cJSON_AddItemToArray(arr, cJSON_CreateNumber(color.red));
		cJSON_AddItemToArray(arr, cJSON_CreateNumber(color.green));
		cJSON_AddItemToArray(arr, cJSON_CreateNumber(color.blue));
	};

	add_rgb("color_on", color_on_.value);
	add_rgb("color_off", color_off_.value);

	auto *range = cJSON_AddArrayToObject(params, "led_range");
	cJSON_AddItemToArray(range, cJSON_CreateNumber(led_range_.value[0]));
	cJSON_AddItemToArray(range, cJSON_CreateNumber(led_range_.value[1]));

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

	add_schema("cycle_time", cycle_time_.schema);
	add_schema("default_brightness", default_brightness_.schema);
	add_schema("led_range", led_range_.schema);
	add_schema("color_on", color_on_.schema);
	add_schema("color_off", color_off_.schema);

	char *json_str = cJSON_PrintUnformatted(root.get());
	EffectParser::cJSON_str_ptr str(json_str, free);

	return FileManager::save_file(path, str.get());
}
