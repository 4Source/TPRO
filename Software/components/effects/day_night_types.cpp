#include "day_night_types.hpp"
#include "esp_log.h"
#include <ArduinoJson.h>
#include <cctype>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <utility>

namespace DayNight {

// ============================================================
// LED CONFIG LADEN
// ============================================================

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
esp_err_t load_day_from_file(const std::string &path) {
	ESP_LOGD("day-night-types", "Stream-Parser: %s", path.c_str());
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

	for (auto &led : *DayNight::shared_led_data) {
		led.sunrise_min = -1;
		led.sunset_min = -1;
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
						sr_minute = -3;
						ss_minute = -3;
					} else if (is_polar_night) {
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
					for (auto &led : *DayNight::shared_led_data) {
						if (std::cmp_equal(led.x_pos, current_x) && std::cmp_equal(led.y_pos, current_y)) {
							led.sunrise_min = static_cast<int16_t>(sr_minute);
							led.sunset_min = static_cast<int16_t>(ss_minute);
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
	ESP_LOGD("day-night-types", "Gelesen: %d JSON-Punkte, %d LED-Matches", eintraege, matches);

	if (matches == 0) {
		ESP_LOGE("day-night-types", "Fehler: Keine passenden LEDs gefunden!");
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t load_leds_streaming(const std::string &path) {
	ESP_LOGD("day-night-types", "Loading LEDs from '%s' via Stream-Parser...", path.c_str());

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
		ESP_LOGE("day-night-types", "Array 'leds' nicht in der Datei gefunden!");
		FileManager::close_file(file);
		return ESP_FAIL;
	}

	while ((ctx = fgetc(file)) != EOF) {
		if (ctx == '[') {
			break;
		}
	}

	DayNight::shared_led_data->clear(); // Nur noch EIN Vektor zu leeren

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
							new_led.sunrise_min = -1; // Standard-Werte setzen
							new_led.sunset_min = -1;

							DayNight::shared_led_data->push_back(new_led);
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

} // namespace DayNight