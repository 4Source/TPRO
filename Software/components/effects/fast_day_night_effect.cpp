#include "effect/fast_day_night_effect.hpp"
#include <ArduinoJson.h>
#include <cctype>
#include <cinttypes>
#include <cmath>
#include <esp_log.h>
#include <esp_timer.h>

FastDayNightEffect::FastDayNightEffect() {
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

esp_err_t FastDayNightEffect::set_filepath(std::string path) {
	this->path_ = path;
	return ESP_OK;
}
std::string FastDayNightEffect::get_filepath() { return this->path_; }

std::string FastDayNightEffect::get_name() { return this->name_; }

esp_err_t FastDayNightEffect::request_api() { return ESP_OK; }

esp_err_t FastDayNightEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	// strcmp liefert 0 bei Gleichheit!
	if (strcmp(name, "speedup") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		speedup_.value = val;
		return ESP_OK;
	}
	if (strcmp(name, "reset") == 0) {
		call_counter_ = 0; // Erzwingt Neusynchronisation mit RTC in get_led_data
		return ESP_OK;
	}
	if (strcmp(name, "default_speed") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0') {
			return ESP_ERR_INVALID_ARG;
		}
		default_speed_.value = val;
		return ESP_OK;
	}
	if (strcmp(name, "default_brightness") == 0) {
		char *endptr = nullptr;
		uint32_t val = strtoul(value, &endptr, 10);
		if (*endptr != '\0' || val > 255) {
			return ESP_ERR_INVALID_ARG;
		}

		default_brightness_.value = static_cast<uint8_t>(val);
		return ESP_OK;
	}

	ESP_LOGW("FastDayNight", "Parameter '%s' unbekannt", name);
	return ESP_ERR_NOT_FOUND; // Deutlicher als INVALID_ARG
}

// -----------------
// JSON
// -----------------
esp_err_t FastDayNightEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);
	if (!opt_json) {
		ESP_LOGE("FastDayNight", "Konnte Config-Datei nicht lesen: %s", path.c_str());
		return ESP_FAIL;
	}

	this->path_ = path;

	esp_err_t parse_err = EffectParser::parse_with_defaults(opt_json->c_str(), path_buffer_.data(), path_buffer_.size(), [this](jparse_ctx_t *jctx) {
		std::array<char, 64> buf{};

		if (json_obj_get_string(jctx, "name", buf.data(), buf.size()) == 0) {
			name_ = buf.data();
		}
		if (json_obj_get_string(jctx, "version", buf.data(), buf.size()) == 0) {
			version = buf.data();
		}

		int tmp = 0;
		if (json_obj_get_int(jctx, "parameters.speedup", &tmp) == 0) {
			speedup_.value = static_cast<uint32_t>(tmp);
		}

		if (json_obj_get_int(jctx, "parameters.default_speed", &tmp) == 0) {
			default_speed_.value = static_cast<uint32_t>(tmp);
		}

		if (json_obj_get_int(jctx, "parameters.default_brightness", &tmp) == 0) {
			default_brightness_.value = static_cast<uint8_t>(tmp);
		}

		return ESP_OK;
	});

	if (parse_err != ESP_OK) {
		return parse_err;
	}

	if (DayNight::load_leds_streaming("led-config.json") != ESP_OK) {
		return ESP_FAIL;
	}

	DateTime::TimeComponents now = DateTime::get_now();
	std::string today_path = build_day_path(now);

	if (DayNight::load_day_from_file(today_path) != ESP_OK) {
		ESP_LOGW("FastDayNight", "Keine Sonnen-Daten gefunden, nutze Fallback");
		for (auto &led : *DayNight::shared_led_data) {
			led.sunrise_min = 360; // 06:00
			led.sunset_min = 1080; // 18:00
		}
	}

	return ESP_OK;
}

// ============================================================
// LED GENERATION
// ============================================================

esp_err_t FastDayNightEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (DayNight::shared_led_data == nullptr || DayNight::shared_led_data->empty()) {
		return ESP_ERR_INVALID_STATE;
	}

	auto now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);

	if (speedup_.value <= 1) {
		accumulated_minutes_ = static_cast<float>((time_stamp.hour * 60) + time_stamp.minute);
		last_millis_ = now_ms;

		last_sim_day_ = 0;
		call_counter_ = 1;
	} else {
		if (call_counter_ == 0) {
			accumulated_minutes_ = static_cast<float>((time_stamp.hour * 60) + time_stamp.minute);
			// Fixiere den Start-Tag, damit add_days stabil bleibt
			start_date_ = DateTime::get_now();
			last_millis_ = now_ms;
			call_counter_ = 1;
		}

		uint32_t delta_ms = now_ms - last_millis_;
		last_millis_ = now_ms;
		accumulated_minutes_ += (static_cast<float>(delta_ms) / 60000.0F) * static_cast<float>(speedup_.value);
	}

	int total_sim_minutes = static_cast<int>(accumulated_minutes_);
	int current_sim_day = total_sim_minutes / 1440;
	int now = total_sim_minutes % 1440;

	// Tageswechsel
	if (current_sim_day != last_sim_day_) {
		last_sim_day_ = current_sim_day;

		DateTime::TimeComponents sim_date = DateTime::add_days(start_date_, current_sim_day);

		std::string path = build_day_path(sim_date);

		// In-Place Update
		DayNight::load_day_from_file(path);
	}
	// 3. LED-Loop mit deiner Logik
	for (const auto &led : *DayNight::shared_led_data) {
		int sun_rise = led.sunrise_min;
		int sun_set = led.sunset_min;

		bool is_day = false;

		// Polartag / Polarnacht / Standard Check
		if (sun_rise == -3 && sun_set == -3) {
			is_day = true; // Polartag
		} else if (sun_rise == -2 && sun_set == -2) {
			is_day = false; // Polarnacht
		} else if (sun_rise >= 0 && sun_set >= 0) {
			// Normale Tag-Nacht Logik (beachtet Mitternachts-Überschreitung)
			is_day = (sun_rise < sun_set) ? (now >= sun_rise && now < sun_set) : (now >= sun_rise || now < sun_set);
		}

		uint8_t val = is_day ? 255 : 0;

		int curr_x = led.x_pos;
		int curr_y = led.y_pos;

		// Bounds-Check und Daten schreiben
		if (curr_x >= 0 && curr_x < CONFIG_LED_COLUMNS && curr_y >= 0 && curr_y < (CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS + CONFIG_LED_CH3_ROWS)) {
			frame.led_data[curr_y][curr_x] = RGB{.red = val, .green = val, .blue = val};
		}
	}

	return ESP_OK;
}

esp_err_t FastDayNightEffect::serialize() {
	// cJSON Smart-Pointer (RAII)
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);
	if (!root) {
		return ESP_ERR_NO_MEM;
	}
	cJSON_AddStringToObject(root.get(), "version", version.c_str());
	cJSON_AddStringToObject(root.get(), "name", name_.c_str());
	cJSON_AddStringToObject(root.get(), "type", kType);

	// ---- PARAMETERS ----
	auto *par_node = cJSON_AddObjectToObject(root.get(), "parameters");
	cJSON_AddNumberToObject(par_node, "default_speed", default_speed_.value);
	cJSON_AddNumberToObject(par_node, "default_brightness", default_brightness_.value);
	cJSON_AddNumberToObject(par_node, "speedup", speedup_.value);

	// ---- SCHEMA ----
	auto *schema = cJSON_AddObjectToObject(root.get(), "schema");

	// Lambda für wiederkehrende Schema-Einträge
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
	add_schema("speedup", speedup_.schema);

	// JSON in String umwandeln und speichern
	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);
	if (!json) {
		return ESP_ERR_NO_MEM;
	}
	ESP_LOGD("FastDayNight", "Speichere Konfiguration nach %s", path_.c_str());
	return FileManager::save_file(this->path_, json.get());
}