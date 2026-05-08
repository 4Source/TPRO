#include "effect/fire_effect.hpp"
#include <algorithm>
#include <span>

esp_err_t FireEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	const float brg_val = static_cast<float>(default_brightness_.value) / 255.0F;

	if (heat_.empty() || heat_[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	// --- Zeitsteuerung einrechnen ---
	// Wir berechnen den aktuellen Zeitstempel in Millisekunden
	const uint32_t current_ms = (time_stamp.second * 1000) + time_stamp.millisecond;
	const uint32_t cycle = std::max<uint32_t>(1, cycle_time_.value);

	// Statische Variable, um den Zeitpunkt des letzten Updates zu speichern
	static uint32_t last_update_ms = 0;

	// Nur wenn die verstrichene Zeit größer als die cycle_time ist, berechnen wir die Hitze neu
	// Ansonsten geben wir einfach den letzten Zustand (im heat_ Array gespeichert) zurück
	bool should_update = (current_ms - last_update_ms) >= cycle;

	if (should_update) {
		last_update_ms = current_ms;

		// 1. Schritt: Funkenbildung (Sparking) am Boden
		std::span<int> last_row_span{heat_.back()};
		for (size_t curr_x = 0; curr_x < WIDTH; ++curr_x) {
			if ((rand() % 255) < sparking_.value) {
				last_row_span[curr_x] = 160 + (rand() % 95);
			} else {
				int cooldown_val = rand() % ((cooling_.value / 5) + 2);
				last_row_span[curr_x] = std::max(0, last_row_span[curr_x] - cooldown_val);
			}
		}

		// 2. Schritt: Hitze nach oben steigen lassen und abkühlen
		for (int curr_y = static_cast<int>(HEIGHT) - 2; curr_y >= 0; --curr_y) {
			std::span<int> cur_row_span{heat_[curr_y]};
			std::span<const int> nxt_row_span{heat_[curr_y + 1]};

			for (size_t curr_x = 0; curr_x < WIDTH; ++curr_x) {
				const int off_val = (scale_.value > 1) ? static_cast<int>(rand() % scale_.value) : 1;

				const int below_val = nxt_row_span[curr_x];
				const int left_val = nxt_row_span[(curr_x + WIDTH - static_cast<size_t>(off_val)) % WIDTH];
				const int right_val = nxt_row_span[(curr_x + static_cast<size_t>(off_val)) % WIDTH];

				int avg_val = (below_val + left_val + right_val) / 3;
				int cooling_limit = (static_cast<int>(cooling_.value) * 10 / static_cast<int>(HEIGHT)) + 2;
				int decay_val = rand() % cooling_limit;

				cur_row_span[curr_x] = std::max(0, avg_val - decay_val);
			}
		}
	}

	// 3. Schritt: Heat-Map in LED-Farben umwandeln (immer ausführen für flüssiges Rendering)
	for (uint32_t curr_y = 0; curr_y < HEIGHT; ++curr_y) {
		std::span<RGB> frame_row_span{frame.led_data[curr_y]};
		std::span<int> cur_row_heat{heat_[curr_y]};

		for (size_t curr_x = 0; curr_x < WIDTH; ++curr_x) {
			size_t pix_index = (static_cast<size_t>(curr_y) * WIDTH) + curr_x;

			if (pix_index >= led_range_start_.value && pix_index <= led_range_end_.value) {
				const auto temp_val = static_cast<float>(cur_row_heat[curr_x]);

				// Klassische Feuer-Farbpalette: Rot -> Gelb -> Weiß
				frame_row_span[curr_x] = RGB{.red = static_cast<uint8_t>(temp_val * brg_val),
											 .green = static_cast<uint8_t>((temp_val / 2.0F) * brg_val),
											 .blue = static_cast<uint8_t>((temp_val / 8.0F) * brg_val)};
			} else {
				frame_row_span[curr_x] = RGB{.red = static_cast<uint8_t>(0), .green = static_cast<uint8_t>(0), .blue = static_cast<uint8_t>(0)};
			}
		}
	}

	return ESP_OK;
}

esp_err_t FireEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = std::move(path);
	return ESP_OK;
}

std::string FireEffect::get_name() { return this->name_; }

std::string FireEffect::get_filepath() { return path_; }

esp_err_t FireEffect::set_parameter(const char *name, const char *value) {
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
	} else if (strcmp(name, "cooling") == 0) {
		cooling_.value = static_cast<uint8_t>(parsed_value);
	} else if (strcmp(name, "sparking") == 0) {
		sparking_.value = static_cast<uint8_t>(parsed_value);
	} else {
		return ESP_ERR_NOT_FOUND;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t FireEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);
	if (!opt_json) { return ESP_FAIL; }
	this->path_ = path;

	EffectParser::cJSON_ptr root(cJSON_Parse(opt_json->c_str()), cJSON_Delete);
	if (!root) { return ESP_FAIL; }

	auto *params = cJSON_GetObjectItem(root.get(), "parameters");
	if (!params) { return ESP_OK; }

	if (auto *item = cJSON_GetObjectItem(params, "default_brightness"); cJSON_IsNumber(item))
		default_brightness_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "cycle_time"); cJSON_IsNumber(item))
		cycle_time_.value = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "speed"); cJSON_IsNumber(item))
		speed_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "scale"); cJSON_IsNumber(item))
		scale_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "cooling"); cJSON_IsNumber(item))
		cooling_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "sparking"); cJSON_IsNumber(item))
		sparking_.value = static_cast<uint8_t>(item->valuedouble);

	if (auto *arr = cJSON_GetObjectItem(params, "led_range"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 2) {
		led_range_start_.value = static_cast<uint32_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		led_range_end_.value   = static_cast<uint32_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
	}

	return ESP_OK;
}

esp_err_t FireEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *par_node = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(par_node, "default_brightness", default_brightness_.value);
	cJSON_AddNumberToObject(par_node, "cycle_time", cycle_time_.value);
	cJSON_AddNumberToObject(par_node, "speed", speed_.value);
	cJSON_AddNumberToObject(par_node, "scale", scale_.value);
	cJSON_AddNumberToObject(par_node, "cooling", cooling_.value);
	cJSON_AddNumberToObject(par_node, "sparking", sparking_.value);

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
	add_schema("cooling", cooling_.schema);
	add_schema("sparking", sparking_.schema);
	add_schema("led_range", led_range_start_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}