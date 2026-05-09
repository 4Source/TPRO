#include "effect/equalizer_effect.hpp"
#include <cstdlib>

esp_err_t EqualizerEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	frame.clear_led_data();
	(void)time_stamp;

	if (heights_.empty()) {
		heights_.resize(frame.led_data[0].size(), 0);
	}

	const size_t width = frame.led_data[0].size();
	const size_t height = frame.led_data.size();

	const uint8_t decay_val = std::max<uint8_t>(1, decay_.value);

	for (size_t curr_x = 0; curr_x < width; ++curr_x) {

		if (rand() % 5 == 0) [[unlikely]] {
			heights_[curr_x] = rand() % height;
		} else if (heights_[curr_x] > 0) {
			heights_[curr_x] -= decay_val;
		}

		for (size_t curr_y = 0; curr_y < height; ++curr_y) {

			if (curr_y < heights_[curr_x]) {

				const size_t inverted_y = height - 1 - curr_y;
				// NOLINTNEXTLINE[cppcoreguidelines-pro-bounds-constant-array-index]
				frame.led_data[inverted_y][curr_x] =
					RGB{.red = static_cast<uint8_t>(0), .green = static_cast<uint8_t>(255), .blue = static_cast<uint8_t>(0)};
			}
		}
	}

	return ESP_OK;
}

esp_err_t EqualizerEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = path;
	return ESP_OK;
}

std::string EqualizerEffect::get_filepath() { return path_; }

std::string EqualizerEffect::get_name() { return this->name_; }

esp_err_t EqualizerEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	if (strcmp(name, "led_range_start") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		led_range_start_.value = val;
	} else if (strcmp(name, "led_range_end") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		led_range_end_.value = val;
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
	} else if (strcmp(name, "decay") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}
		decay_.value = static_cast<uint8_t>(val);
	} else {
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t EqualizerEffect::deserialize(std::string path) {
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

	if (auto *item = cJSON_GetObjectItem(params, "decay"); cJSON_IsNumber(item))
		decay_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "speed"); cJSON_IsNumber(item))
		speed_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "scale"); cJSON_IsNumber(item))
		scale_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "led_range_start"); cJSON_IsNumber(item))
		led_range_start_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "led_range_end"); cJSON_IsNumber(item))
		led_range_end_.value = static_cast<uint32_t>(item->valuedouble);

	return ESP_OK;
}

esp_err_t EqualizerEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *par_node = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(par_node, "decay", decay_.value);
	cJSON_AddNumberToObject(par_node, "speed", speed_.value);
	cJSON_AddNumberToObject(par_node, "scale", scale_.value);
	cJSON_AddNumberToObject(par_node, "led_range_start", led_range_start_.value);
	cJSON_AddNumberToObject(par_node, "led_range_end", led_range_end_.value);

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

	add_schema("decay", decay_.schema);
	add_schema("speed", speed_.schema);
	add_schema("scale", scale_.schema);
	add_schema("led_range_start", led_range_start_.schema);
	add_schema("led_range_end", led_range_end_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}