#include "effect/matrix_effect.hpp"
#include <cstdlib>

esp_err_t MatrixEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	frame.clear_led_data();
	(void)time_stamp; // ungenutzt im Matrix-Effekt, da er Frame-basiert läuft

	const size_t width = frame.led_data[0].size();
	const size_t height = frame.led_data.size();

	// Initiale Zuweisung des drops_ Vectors, falls er leer ist oder die Breite nicht stimmt
	if (drops_.size() != width) {
		drops_.assign(width, 0);
	}

	// Helligkeits-Faktor (0.0 - 1.0)
	const float brightness = static_cast<float>(default_brightness_.value) / 255.0F;

	// Die Scale steuert die Schweiflänge (Minimum 1)
	const int tail_length = std::max(1, 5 * static_cast<int>(scale_.value));

	// Sicherheits-Check, damit modulo 0 nicht crasht
	const int safe_spawn_rate = (spawn_rate_.value > 0) ? spawn_rate_.value : 1;

	for (size_t curr_x = 0; curr_x < width; ++curr_x) {
		// Neuen Tropfen starten
		if (rand() % safe_spawn_rate == 0) {
			drops_[curr_x] = 0;
		}

		const int head = drops_[curr_x];

		for (size_t curr_y = 0; curr_y < height; ++curr_y) {
			size_t pixel_index = (curr_y * width) + curr_x;

			if (pixel_index >= led_range_start_.value && pixel_index <= led_range_end_.value) {
				const int dist = head - static_cast<int>(curr_y);

				if (dist == 0) {
					// Kopf des Tropfens (Hellgrün/Weiß)
					// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
					frame.led_data[curr_y][curr_x] = RGB{.red = static_cast<uint8_t>(180.0F * brightness),
														 .green = static_cast<uint8_t>(255.0F * brightness),
														 .blue = static_cast<uint8_t>(180.0F * brightness)};
				} else if (dist > 0 && dist < tail_length) {
					// Fade-Out des Schweifs basierend auf tail_length
					float fade_factor = 1.0F - (static_cast<float>(dist) / static_cast<float>(tail_length));
					auto green_val = static_cast<uint8_t>(255.0F * fade_factor * brightness);

					// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
					frame.led_data[curr_y][curr_x] = RGB{.red = 0, .green = green_val, .blue = 0};
				} else {
					// Schwarz
					// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
					frame.led_data[curr_y][curr_x] = RGB{.red = 0, .green = 0, .blue = 0};
				}
			} else {
				// Pixel außerhalb der Range bleiben dunkel
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				frame.led_data[curr_y][curr_x] = RGB{.red = 0, .green = 0, .blue = 0};
			}
		}

		// Tropfen weiter nach unten schieben (speed_ = 1 ist flüssig, speed_ = 2 überspringt Pixel)
		drops_[curr_x] += (speed_.value > 0) ? speed_.value : 1;
	}

	return ESP_OK;
}

esp_err_t MatrixEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = path;
	return ESP_OK;
}

std::string MatrixEffect::get_filepath() { return path_; }

std::string MatrixEffect::get_name() { return this->name_; }

esp_err_t MatrixEffect::set_parameter(const char *name, const char *value) {
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
	} else if (strcmp(name, "spawn_rate") == 0) {
		spawn_rate_.value = static_cast<uint8_t>(parsed_value);
	} else {
		return ESP_ERR_NOT_FOUND;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t MatrixEffect::deserialize(std::string path) {
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

	if (auto *item = cJSON_GetObjectItem(params, "default_brightness"); cJSON_IsNumber(item))
		default_brightness_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "cycle_time"); cJSON_IsNumber(item))
		cycle_time_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "speed"); cJSON_IsNumber(item))
		speed_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "scale"); cJSON_IsNumber(item))
		scale_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "spawn_rate"); cJSON_IsNumber(item))
		spawn_rate_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *arr = cJSON_GetObjectItem(params, "led_range"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 2) {
		led_range_start_.value = static_cast<uint32_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		led_range_end_.value = static_cast<uint32_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
	}

	return ESP_OK;
}

esp_err_t MatrixEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *par_node = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(par_node, "default_brightness", default_brightness_.value);
	cJSON_AddNumberToObject(par_node, "cycle_time", cycle_time_.value);
	cJSON_AddNumberToObject(par_node, "speed", speed_.value);
	cJSON_AddNumberToObject(par_node, "scale", scale_.value);
	cJSON_AddNumberToObject(par_node, "spawn_rate", spawn_rate_.value);

	auto *rng_arr = cJSON_AddArrayToObject(par_node, "led_range");

	cJSON_AddItemToArray(rng_arr, cJSON_CreateNumber(led_range_start_.value));

	cJSON_AddItemToArray(rng_arr, cJSON_CreateNumber(led_range_end_.value));

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
	add_schema("spawn_rate", spawn_rate_.schema);
	add_schema("led_range", led_range_start_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}