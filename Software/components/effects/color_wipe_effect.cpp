#include "effect/color_wipe_effect.hpp"

esp_err_t ColorWipeEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {

	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	frame.clear_led_data();
	const uint64_t ms_time = ((time_stamp.second * 1000) + time_stamp.millisecond);

	const size_t max_leds = frame.led_data.size() * frame.led_data[0].size();

	const uint32_t spd_val = std::max<uint32_t>(1, speed_.value);

	const size_t prog_val = (ms_time / spd_val) % max_leds;

	size_t idx_val = 0;

	const uint32_t led_start = led_start_.value;
	const uint32_t led_end = led_end_.value;

	for (auto &row_data : frame.led_data) {
		for (auto &led_val : row_data) {

			if (idx_val < led_start || idx_val > led_end) {
				led_val = col_b_.value;
			} else {
				led_val = (idx_val < prog_val) ? col_a_.value : col_b_.value;
			}

			idx_val++;
		}
	}

	return ESP_OK;
}

esp_err_t ColorWipeEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = path;
	return ESP_OK;
}

std::string ColorWipeEffect::get_filepath() { return path_; }

std::string ColorWipeEffect::get_name() { return this->name_; }

esp_err_t ColorWipeEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	if (strcmp(name, "bri_default") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}
		bri_default_.value = static_cast<uint8_t>(val);
	} else if (strcmp(name, "cyc_time") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		cyc_time_.value = val;
	} else if (strcmp(name, "led_start") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		led_start_.value = val;
	} else if (strcmp(name, "led_end") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		led_end_.value = val;
	} else if (strcmp(name, "speed") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}
		speed_.value = static_cast<uint8_t>(val);
	} else if (strcmp(name, "scale") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}
		scale_.value = static_cast<uint8_t>(val);
	} else if (strcmp(name, "col_a") == 0) {
		uint8_t red = 0;
		uint8_t green = 0;
		uint8_t blue = 0;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		col_a_.value = RGB{.red = red, .green = green, .blue = blue};
	} else if (strcmp(name, "col_b") == 0) {
		uint8_t red = 0;
		uint8_t green = 0;
		uint8_t blue = 0;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		col_b_.value = RGB{.red = red, .green = green, .blue = blue};
	} else {
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t ColorWipeEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);
	if (!opt_json) {
		return ESP_FAIL;
	}
	this->path_ = path;

	EffectParser::cJSON_ptr root(cJSON_Parse(opt_json->c_str()), cJSON_Delete);
	if (!root) {
		return ESP_FAIL;
	}

	auto *params = cJSON_GetObjectItem(root.get(), "parameters");
	if (!params) {
		return ESP_OK;
	}

	if (auto *item = cJSON_GetObjectItem(params, "speed"); cJSON_IsNumber(item))
		speed_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "cycle_time"); cJSON_IsNumber(item))
		cyc_time_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "led_range_start"); cJSON_IsNumber(item))
		led_start_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "led_range_end"); cJSON_IsNumber(item))
		led_end_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *arr = cJSON_GetObjectItem(params, "color_a"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 3) {
		col_a_.value.red = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		col_a_.value.green = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
		col_a_.value.blue = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 2)->valuedouble);
	}

	if (auto *arr = cJSON_GetObjectItem(params, "color_b"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 3) {
		col_b_.value.red = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		col_b_.value.green = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
		col_b_.value.blue = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 2)->valuedouble);
	}

	return ESP_OK;
}

esp_err_t ColorWipeEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *par_node = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(par_node, "speed", speed_.value);
	cJSON_AddNumberToObject(par_node, "cycle_time", cyc_time_.value);
	cJSON_AddNumberToObject(par_node, "led_range_start", led_start_.value);
	cJSON_AddNumberToObject(par_node, "led_range_end", led_end_.value);

	auto *col_a_arr = cJSON_AddArrayToObject(par_node, "color_a");
	cJSON_AddItemToArray(col_a_arr, cJSON_CreateNumber(col_a_.value.red));
	cJSON_AddItemToArray(col_a_arr, cJSON_CreateNumber(col_a_.value.green));
	cJSON_AddItemToArray(col_a_arr, cJSON_CreateNumber(col_a_.value.blue));

	auto *col_b_arr = cJSON_AddArrayToObject(par_node, "color_b");
	cJSON_AddItemToArray(col_b_arr, cJSON_CreateNumber(col_b_.value.red));
	cJSON_AddItemToArray(col_b_arr, cJSON_CreateNumber(col_b_.value.green));
	cJSON_AddItemToArray(col_b_arr, cJSON_CreateNumber(col_b_.value.blue));

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

	add_schema("speed", speed_.schema);
	add_schema("cycle_time", cyc_time_.schema);
	add_schema("led_range_start", led_start_.schema);
	add_schema("led_range_end", led_end_.schema);
	add_schema("color_a", col_a_.schema);
	add_schema("color_b", col_b_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}