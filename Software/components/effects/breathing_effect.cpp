#include "effect/breathing_effect.hpp"
#include <cmath>
#include <esp_err.h>

constexpr float kPi = std::numbers::pi_v<float>;

esp_err_t BreathingEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	const uint32_t cycle = std::max<uint32_t>(1, cycle_time_.value);

	const auto speed = static_cast<float>(speed_.value);
	const auto scale = static_cast<float>(scale_.value);

	const float time = (static_cast<float>(((time_stamp.second * 1000) + time_stamp.millisecond)) * speed) / static_cast<float>(cycle);

	const float t_mod = std::fmod(time, 1.0F);

	// breathing curve
	float brightness = (std::sin(t_mod * 2.0F * std::numbers::pi_v<float>) + 1.0F) * 0.5F;

	brightness = std::pow(brightness, scale); // shaping

	const RGB &base = color_.value;

	const float breathing = static_cast<float>(default_brightness_.value) / 255.0F;

	RGB current{.red = static_cast<uint8_t>(static_cast<float>(base.red) * brightness * breathing),
				.green = static_cast<uint8_t>(static_cast<float>(base.green) * brightness * breathing),
				.blue = static_cast<uint8_t>(static_cast<float>(base.blue) * brightness * breathing)};

	for (auto &row : frame.led_data) {
		for (auto &led : row) {
			led = current;
		}
	}

	return ESP_OK;
}

esp_err_t BreathingEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = path;
	return ESP_OK;
}

std::string BreathingEffect::get_filepath() { return path_; }

std::string BreathingEffect::get_name() { return this->name_; }

esp_err_t BreathingEffect::set_parameter(const char *name, const char *value) {
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
	} else if (strcmp(name, "color") == 0) {
		uint8_t red = 0;
		uint8_t green = 0;
		uint8_t blue = 0;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_.value = RGB{.red = red, .green = green, .blue = blue};
	} else {
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t BreathingEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);
	if (!opt_json) { return ESP_FAIL; }
	this->path_ = path;

	EffectParser::cJSON_ptr root(cJSON_Parse(opt_json->c_str()), cJSON_Delete);
	if (!root) { return ESP_FAIL; }

	if (auto *item = cJSON_GetObjectItem(root.get(), "name"); cJSON_IsString(item))
		name_ = item->valuestring;

	auto *params = cJSON_GetObjectItem(root.get(), "parameters");
	if (!params) { return ESP_OK; }

	if (auto *item = cJSON_GetObjectItem(params, "cycle_time"); cJSON_IsNumber(item))
		cycle_time_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "default_brightness"); cJSON_IsNumber(item))
		default_brightness_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "speed"); cJSON_IsNumber(item))
		speed_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "scale"); cJSON_IsNumber(item))
		scale_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *arr = cJSON_GetObjectItem(params, "color"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 3) {
		color_.value.red   = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		color_.value.green = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
		color_.value.blue  = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 2)->valuedouble);
	}

	return ESP_OK;
}

esp_err_t BreathingEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *params = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(params, "cycle_time", cycle_time_.value);
	cJSON_AddNumberToObject(params, "default_brightness", default_brightness_.value);
	cJSON_AddNumberToObject(params, "speed", speed_.value);
	cJSON_AddNumberToObject(params, "scale", scale_.value);

	auto *col = cJSON_AddArrayToObject(params, "color");
	cJSON_AddItemToArray(col, cJSON_CreateNumber(color_.value.red));
	cJSON_AddItemToArray(col, cJSON_CreateNumber(color_.value.green));
	cJSON_AddItemToArray(col, cJSON_CreateNumber(color_.value.blue));

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
	add_schema("speed", speed_.schema);
	add_schema("scale", scale_.schema);
	add_schema("color", color_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}