#include "effect/day_night_effect.hpp"
#include <ArduinoJson.h>
#include <cctype>
#include <cinttypes>
#include <cmath>
#include <esp_log.h>
#include <format>
#include <utility>

DayNightEffect::DayNightEffect() {
	_led_data = std::make_shared<std::vector<DayNightData>>();
	_led_data->reserve(3044);
}

// ============================================================
// HELPER
// ============================================================

static std::string build_day_path(const DateTime::TimeComponents &now) {
	return std::format("sun-data/2024-{:02}-{:02}.json", static_cast<int>(now.month), static_cast<int>(now.day));
}

esp_err_t DayNightEffect::set_filepath(std::string path) {
	this->path_ = path;
	return ESP_OK;
}
std::string DayNightEffect::get_filepath() { return this->path_; }

std::string DayNightEffect::get_name() { return this->name_; }

esp_err_t DayNightEffect::request_api() { return ESP_OK; }

esp_err_t DayNightEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	if (strcmp(name, "default_speed") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		default_speed_.value = val;
	} else if (strcmp(name, "default_brightness") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}
		default_brightness_.value = static_cast<uint8_t>(val);
	} else {
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t DayNightEffect::deserialize(std::string path) {
	path_ = path;
	if (load_leds_streaming("led-config.json") != ESP_OK) {
		return ESP_FAIL;
	}

	DateTime::TimeComponents now = DateTime::get_now();
	std::string today_path = build_day_path(now);

	if (load_day_from_file(today_path) == ESP_OK) {
		return ESP_OK;
	}

	ESP_LOGW(Effect::kTag, "Using fallback dummy data");

	// Fallback: Alle Einträge im Array auf Dummy-Werte setzen
	for (auto &led : *_led_data) {
		led.sunrise_minute = 360;
		led.sunset_minute = 1080;
	}

	return ESP_OK;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
esp_err_t DayNightEffect::load_day_from_file(const std::string &path) {
	ESP_LOGI(Effect::kTag, "Stream-Parser: %s", path.c_str());
	FILE *file = FileManager::open_file(path, "rb");
	if (file == nullptr) {
		return ESP_FAIL;
	}

	int monat = 1;
	size_t dash1 = path.find('-');
	size_t dash2 = path.find('-', dash1 + 1);
	if (dash2 != std::string::npos && dash2 + 3 <= path.length()) {
		monat = atoi(path.substr(dash2 + 1, 2).c_str());
	}

	for (auto &led : *_led_data) {
		led.sunrise_minute = -1;
		led.sunset_minute = -1;
	}

	int eintraege = 0;
	int matches = 0;
	int ctx = 0;
	int brace_level = 0;
	int current_x = -1;
	int current_y = -1;
	bool inside_key = false;
	std::string current_key;
	std::string json_buffer;
	json_buffer.reserve(128);
	JsonDocument doc;

	while ((ctx = fgetc(file)) != EOF) {
		if (isspace(ctx) != 0) {
			continue;
		}
		if (brace_level == 3) {
			json_buffer += static_cast<char>(ctx);
			if (ctx == '}') {
				brace_level--;
				doc.clear();
				if (!deserializeJson(doc, json_buffer) && current_x != -1 && current_y != -1) {
					eintraege++;

					int hour_sunrise = -1;
					int minute_sunrise = -1;
					int second_sunrise = -1;
					int hour_sunset = -1;
					int minute_sunset = -1;
					int second_sunset = -1;
					bool is_polar_day = false;
					bool is_polar_night = false;
					is_polar_day = (doc["sr"] == "01:00:01" && doc["ss"] == "01:00:01");
					is_polar_night = (doc["sr"] == "01:00:00" && doc["ss"] == "01:00:00");

					int sr_minute = 0;
					int ss_minute = 0;

					if (is_polar_day) {
						ESP_LOGI("DayNightEffect", "Polar Tag erkannt bei X:%d Y:%d", current_x, current_y);
						sr_minute = -3;
						ss_minute = -3;
					} else if (is_polar_night) {
						ESP_LOGI("DayNightEffect", "Polar Nacht erkannt bei X:%d Y:%d", current_x, current_y);
						sr_minute = -2;
						ss_minute = -2;
						// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg)
					} else if (sscanf(doc["sr"] | "", "%d:%d:%d", &hour_sunrise, &minute_sunrise, &second_sunrise) == 3 &&
							   sscanf(doc["ss"] | "", "%d:%d:%d", &hour_sunset, &minute_sunset, &second_sunset) == 3) {
						// NOLINTEND(cppcoreguidelines-pro-type-vararg)

						sr_minute = hour_sunrise * 60 + minute_sunrise;
						ss_minute = hour_sunset * 60 + minute_sunset;

						if (sr_minute == ss_minute) {
							sr_minute = ((current_y < 17) == (monat >= 3 && monat <= 9)) ? 0 : 1440;
							ss_minute = (sr_minute == 0) ? 1440 : 0;
						}
					} else {
						return ESP_ERR_INVALID_ARG; // ungültige Daten
					}

					// Werte der entsprechenden LED zuweisen
					for (auto &led : *_led_data) {
						if (std::cmp_equal(led.x_pos, current_x) && std::cmp_equal(led.y_pos, current_y)) {
							led.sunrise_minute = static_cast<int16_t>(sr_minute);
							led.sunset_minute = static_cast<int16_t>(ss_minute);
							matches++;
							break;
						}
					}
				}
			}
			continue;
		}

		if (ctx == '{') {
			brace_level++;
			if (brace_level == 3) {
				json_buffer = "{";
			}
		} else if (ctx == '}') {
			brace_level--;
			if (brace_level == 1) {
				current_y = -1;
			}
			if (brace_level == 0) {
				current_x = -1;
			}
		} else if (ctx == '"') {
			inside_key = !inside_key;
			if (!inside_key && !current_key.empty()) {
				if (brace_level == 1) {
					current_x = atoi(current_key.c_str());
				}
				if (brace_level == 2) {
					current_y = atoi(current_key.c_str());
				}
			}
			current_key.clear();
		} else if (inside_key) {
			current_key += static_cast<char>(ctx);
		}
	}

	FileManager::close_file(file);
	ESP_LOGI(Effect::kTag, "Gelesen: %d JSON-Punkte, %d LED-Matches", eintraege, matches);

	if (matches == 0) {
		ESP_LOGE(Effect::kTag, "Fehler: Keine passenden LEDs gefunden!");
		return ESP_FAIL;
	}
	return ESP_OK;
}

// ============================================================
// LED GENERATION
// ============================================================

esp_err_t DayNightEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {

	if (!_led_data) {
		return ESP_ERR_INVALID_ARG;
	}
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	// Prüfe alle 1000 Aufrufe ob Tageswechsel
	if (call_counter_++ >= 1000 || last_loaded_day_ == -1) {
		call_counter_ = 0;

		if (std::cmp_not_equal(time_stamp.day, last_loaded_day_)) {
			ESP_LOGI("DayNightEffect", "Prüfe Tageswechsel: %02d.%02d. (alt: %d)", time_stamp.day, time_stamp.month, last_loaded_day_);

			std::string today_path = build_day_path(time_stamp);

			if (load_day_from_file(today_path) == ESP_OK) {
				last_loaded_day_ = static_cast<int8_t>(time_stamp.day);
				ESP_LOGI("DayNightEffect", "Neue Tagesdaten erfolgreich geladen.");
			} else {
				ESP_LOGE("DayNightEffect", "Datei nicht gefunden: %s. Nutze bestehende Daten weiter.", today_path.c_str());
			}
		}
	}

	int now = (time_stamp.hour * 60) + time_stamp.minute;
	for (const auto &led : *_led_data) {
		int led_sunrise = led.sunrise_minute;
		int led_sunset = led.sunset_minute;

		bool is_day = false;

		if (led_sunrise == -3 && led_sunset == -3) {
			is_day = true; // Polartag
		} else if (led_sunrise == -2 && led_sunset == -2) {
			is_day = false; // Polarnacht
		} else {
			is_day = (led_sunrise < led_sunset) ? (now >= led_sunrise && now < led_sunset) : (now >= led_sunrise || now < led_sunset);
		}
		uint8_t val = is_day ? 255 : 0;

		int led_x = led.x_pos;
		int led_y = led.y_pos;

		if (led_x >= 0 && led_x < CONFIG_LED_COLUMNS && led_y >= 0 && led_y < (CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS + CONFIG_LED_CH3_ROWS)) {
			frame.led_data[led_y][led_x] = RGB{.red = val, .green = val, .blue = val};
		}
	}
	return ESP_OK;
}

// ============================================================
// LED CONFIG LADEN
// ============================================================

esp_err_t DayNightEffect::load_leds_streaming(const std::string &path) {
	ESP_LOGI(Effect::kTag, "Loading LEDs from '%s' via Stream-Parser...", path.c_str());

	FILE *file = FileManager::open_file(path, "rb");
	if (file == nullptr) {
		return ESP_FAIL;
	}

	const char *target = "\"leds\"";
	int match_idx = 0;
	int ctx = 0;
	bool found_leds = false;

	while ((ctx = fgetc(file)) != EOF) {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		if (ctx == target[match_idx]) {
			match_idx++;
			if (match_idx == 6) {
				found_leds = true;
				break;
			}
		} else {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			match_idx = (ctx == target[0]) ? 1 : 0;
		}
	}

	if (!found_leds) {
		ESP_LOGE(Effect::kTag, "Array 'leds' nicht in der Datei gefunden!");
		FileManager::close_file(file);
		return ESP_FAIL;
	}

	while ((ctx = fgetc(file)) != EOF) {
		if (ctx == '[') {
			break;
		}
	}

	_led_data->clear(); // Nur noch EIN Vektor zu leeren

	int brace_level = 0;
	std::string json_buffer;
	json_buffer.reserve(128);
	JsonDocument doc;

	while ((ctx = fgetc(file)) != EOF) {
		if (ctx == '{') {
			brace_level++;
			json_buffer += static_cast<char>(ctx);
		} else if (ctx == '}') {
			if (brace_level > 0) {
				brace_level--;
				json_buffer += static_cast<char>(ctx);
				if (brace_level == 0) {
					doc.clear();
					if (!deserializeJson(doc, json_buffer)) {
						if (doc["x"].is<int>() && doc["y"].is<int>()) {

							// Strukturiertes Einfügen in den Vektor
							DayNightData new_led{};
							new_led.x_pos = doc["x"] | 0;
							new_led.y_pos = doc["y"] | 0;
							new_led.lat = doc["lat"] | 0.0F;
							new_led.lon = doc["lon"] | 0.0F;
							new_led.sunrise_minute = -1; // Standard-Werte setzen
							new_led.sunset_minute = -1;

							_led_data->push_back(new_led);
						}
					}
					json_buffer.clear();
				}
			}
		} else if (brace_level > 0) {
			json_buffer += static_cast<char>(ctx);
		} else if (ctx == ']') {
			break;
		}
	}

	FileManager::close_file(file);
	return ESP_OK;
}

esp_err_t DayNightEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());
	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *par_node = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddNumberToObject(par_node, "default_speed", default_speed_.value);
	cJSON_AddNumberToObject(par_node, "default_brightness", default_brightness_.value);

	// ---- SCHEMA ----
	auto *schema = cJSON_AddObjectToObject(root.get(), "schema");

	auto add_schema = [&](const char *key, const ParameterSchema &schema_obj) {
		auto *schema_entry = cJSON_AddObjectToObject(schema, key);
		cJSON_AddStringToObject(schema_entry, "type", schema_obj.type.c_str());

		if (schema_obj.type == "number" || schema_obj.type == "range") {
			cJSON_AddNumberToObject(schema_entry, "min", schema_obj.min);
			cJSON_AddNumberToObject(schema_entry, "max", schema_obj.max);
			cJSON_AddNumberToObject(schema_entry, "step", schema_obj.step);
		}
	};

	add_schema("default_speed", default_speed_.schema);
	add_schema("default_brightness", default_brightness_.schema);
	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}