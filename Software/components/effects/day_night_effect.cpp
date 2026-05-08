#include "effect/day_night_effect.hpp"
#include <ArduinoJson.h>
#include <cctype>
#include <cinttypes>
#include <cmath>
#include <esp_log.h>
#include <format>
#include <utility>

DayNightEffect::DayNightEffect() {
	if (DayNight::shared_led_data == nullptr) {
		DayNight::shared_led_data = std::make_shared<std::vector<DayNight::DayNightData>>();
		DayNight::shared_led_data->reserve(3044);
	}
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
	if (DayNight::load_leds_streaming("led-config.json") != ESP_OK) {
		return ESP_FAIL;
	}

	DateTime::TimeComponents now = DateTime::get_now();
	std::string today_path = build_day_path(now);

	if (DayNight::load_day_from_file(today_path) == ESP_OK) {
		return ESP_OK;
	}

	ESP_LOGW(Effect::kTag, "Using fallback dummy data");

	// Fallback: Alle Einträge im Array auf Dummy-Werte setzen
	for (auto &led : *DayNight::shared_led_data) {
		led.sunrise_min = 360;
		led.sunset_min = 1080;
	}

	return ESP_OK;
}
// ============================================================
// LED GENERATION
// ============================================================

esp_err_t DayNightEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {

	if (DayNight::shared_led_data == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	// Prüfe alle 1000 Aufrufe ob Tageswechsel
	if (call_counter_++ >= 1000 || last_loaded_day_ == -1) {
		call_counter_ = 0;

		if (std::cmp_not_equal(time_stamp.day, last_loaded_day_)) {
			ESP_LOGD("DayNightEffect", "Prüfe Tageswechsel: %02d.%02d. (alt: %d)", time_stamp.day, time_stamp.month, last_loaded_day_);

			std::string today_path = build_day_path(time_stamp);

			if (DayNight::load_day_from_file(today_path) == ESP_OK) {
				last_loaded_day_ = static_cast<int8_t>(time_stamp.day);
				ESP_LOGD("DayNightEffect", "Neue Tagesdaten erfolgreich geladen.");
			} else {
				ESP_LOGE("DayNightEffect", "Datei nicht gefunden: %s. Nutze bestehende Daten weiter.", today_path.c_str());
			}
		}
	}

	int now = (time_stamp.hour * 60) + time_stamp.minute;
	for (const auto &led : *DayNight::shared_led_data) {
		int led_sunrise = led.sunrise_min;
		int led_sunset = led.sunset_min;

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