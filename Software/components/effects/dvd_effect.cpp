#include "effect/dvd_effect.hpp"

esp_err_t DVDEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	frame.clear_led_data();
	(void)time_stamp;

	pos_x_.value += vel_x_.value;
	pos_y_.value += vel_y_.value;

	if (pos_x_.value <= 0 || pos_x_.value >= WIDTH - 2) [[unlikely]] {
		vel_x_.value *= -1;
	}

	if (pos_y_.value <= 0 || pos_y_.value >= HEIGHT - 2) [[unlikely]] {
		vel_y_.value *= -1;
	}

	const RGB &col_val = col_rgb_.value;

	for (uint32_t cur_y = 0; cur_y < 2; ++cur_y) {
		for (uint32_t cur_x = 0; cur_x < 4; ++cur_x) {
			// NOLINTNEXTLINE[cppcoreguidelines-pro-bounds-constant-array-index]
			frame.led_data[pos_y_.value + cur_y][pos_x_.value + cur_x] = col_val;
		}
	}

	return ESP_OK;
}

esp_err_t DVDEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = path;
	return ESP_OK;
}

std::string DVDEffect::get_filepath() { return path_; }

std::string DVDEffect::get_name() { return this->name_; }

esp_err_t DVDEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	if (strcmp(name, "default_brightness") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}
		default_brightness_.value = static_cast<uint8_t>(val);
	} else if (strcmp(name, "speed") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}
		speed_.value = static_cast<uint8_t>(val);
	} else if (strcmp(name, "vel_x") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		vel_x_.value = val;
	} else if (strcmp(name, "vel_y") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		vel_y_.value = val;
	} else if (strcmp(name, "pos_x") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		pos_x_.value = val;
	} else if (strcmp(name, "pos_y") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		pos_y_.value = val;
	} else if (strcmp(name, "col_rgb") == 0) {
		uint8_t red = 0;
		uint8_t green = 0;
		uint8_t blue = 0;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		col_rgb_.value = RGB{.red = red, .green = green, .blue = blue};
	} else {
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t DVDEffect::deserialize(std::string path) {
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

	if (auto *item = cJSON_GetObjectItem(params, "vel_x"); cJSON_IsNumber(item))
		vel_x_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "vel_y"); cJSON_IsNumber(item))
		vel_y_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "pos_x"); cJSON_IsNumber(item))
		pos_x_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "pos_y"); cJSON_IsNumber(item))
		pos_y_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *arr = cJSON_GetObjectItem(params, "color"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 3) {
		col_rgb_.value.red = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		col_rgb_.value.green = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
		col_rgb_.value.blue = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 2)->valuedouble);
	}

	return ESP_OK;
}

esp_err_t DVDEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *par_node = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(par_node, "speed", speed_.value);
	cJSON_AddNumberToObject(par_node, "vel_x", vel_x_.value);
	cJSON_AddNumberToObject(par_node, "vel_y", vel_y_.value);
	cJSON_AddNumberToObject(par_node, "pos_x", pos_x_.value);
	cJSON_AddNumberToObject(par_node, "pos_y", pos_y_.value);

	auto *col_arr = cJSON_AddArrayToObject(par_node, "color");
	cJSON_AddItemToArray(col_arr, cJSON_CreateNumber(col_rgb_.value.red));
	cJSON_AddItemToArray(col_arr, cJSON_CreateNumber(col_rgb_.value.green));
	cJSON_AddItemToArray(col_arr, cJSON_CreateNumber(col_rgb_.value.blue));

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
	add_schema("vel_x", vel_x_.schema);
	add_schema("vel_y", vel_y_.schema);
	add_schema("pos_x", pos_x_.schema);
	add_schema("pos_y", pos_y_.schema);
	add_schema("color", col_rgb_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}