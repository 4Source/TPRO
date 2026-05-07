#include "effect/continent_effect.hpp"
#include "file_manager.hpp"
#include <ArduinoJson.h>
#include <esp_log.h>
#include <utility>

namespace {
struct PosixFileReader {
	FILE *file;
	PosixFileReader(FILE *file) : file(file) {}
	[[nodiscard]] int read() const { return file != nullptr ? fgetc(file) : -1; }
	size_t read_bytes(char *buffer, size_t length) const { return file != nullptr ? fread(buffer, 1, length, file) : 0; }
};

struct PosixFileWriter {
	FILE *file;
	PosixFileWriter(FILE *file) : file(file) {}
	[[nodiscard]] size_t write(uint8_t byte) const { return file != nullptr ? fwrite(&byte, 1, 1, file) : 0; }
	size_t write(const uint8_t *buffer, size_t length) const { return file != nullptr ? fwrite(buffer, 1, length, file) : 0; }
};
} // namespace

ContinentEffect::ContinentEffect() {
	// Kompakten Vektor für Landmassen erstellen
	this->_land_pixels = std::make_shared<std::vector<ContinentNode>>();
	this->_land_pixels->reserve(1200); // Reserviere Platz für geschätzt 1200 Land-LEDs

	ESP_LOGI(Effect::kTag, "Loading continent mapping from 'led-config.json'...");

	FILE *file = FileManager::open_file("led-config.json", "rb");
	if (file == nullptr) {
		ESP_LOGW(Effect::kTag, "'led-config.json' not found! Failed to read out continent values!");
		return;
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
		ESP_LOGW(Effect::kTag, "Array 'leds' nicht in der Datei gefunden!");
		FileManager::close_file(file);
		return;
	}

	while ((ctx = fgetc(file)) != EOF) {
		if (ctx == '[') {
			break;
		}
	}

	int led_count = 0;
	int brace_level = 0;
	std::string json_buffer;
	json_buffer.reserve(512);
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
					DeserializationError err = deserializeJson(doc, json_buffer);

					if (!err && doc["x"].is<uint32_t>() && doc["y"].is<uint32_t>()) {
						JsonObject continents = doc["continents"];

						if (!continents.isNull()) {
							// Floats (0.0-1.0) einlesen
							float f_north = continents["North"] | 0.0F;
							float f_south = continents["South"] | 0.0F;
							float f_europe = continents["Europe"] | 0.0F;
							float f_africa = continents["Africa"] | 0.0F;
							float f_asia = continents["Asia"] | 0.0F;
							float f_australia = continents["Australia"] | 0.0F;

							// Ist überhaupt Land auf dieser LED?
							if (f_north > 0 || f_south > 0 || f_europe > 0 || f_africa > 0 || f_asia > 0 || f_australia > 0) {
								ContinentNode node{};
								node.x = doc["x"] | 0;
								node.y = doc["y"] | 0;

								// Float zu speicherfreundlichem uint8_t (0-255) umwandeln
								node.North = static_cast<uint8_t>(f_north * 255.0F);
								node.South = static_cast<uint8_t>(f_south * 255.0F);
								node.Europe = static_cast<uint8_t>(f_europe * 255.0F);
								node.Africa = static_cast<uint8_t>(f_africa * 255.0F);
								node.Asia = static_cast<uint8_t>(f_asia * 255.0F);
								node.Australia = static_cast<uint8_t>(f_australia * 255.0F);

								this->_land_pixels->push_back(node);
								led_count++;
							}
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

	// Speicher-Feinschliff: Gib RAM zurück, falls wir weniger als 1200 LEDs gefunden haben
	this->_land_pixels->shrink_to_fit();

	FileManager::close_file(file);
	ESP_LOGI(Effect::kTag, "Mapping geladen! %d Land-LEDs extrem speichersparend hinterlegt.", led_count);
}

// Fallback, falls setWeight in der Applikation noch aufgerufen wird
esp_err_t ContinentEffect::setWeight(int pos_x, int pos_y, const ContinentNode &weight) {
	if (!_land_pixels) {
		return ESP_ERR_INVALID_STATE;
	}

	for (auto &node : *_land_pixels) {
		if (std::cmp_equal(node.x, pos_x) && std::cmp_equal(node.y, pos_y)) {
			node = weight;
			return ESP_OK;
		}
	}
	_land_pixels->push_back(weight);
	return ESP_OK;
}

esp_err_t ContinentEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	// 1. Ganze Karte mit Ozean füllen (schnell)
	for (auto &led_row : frame.led_data) {
		for (auto &led : led_row) {
			led = this->color_ocean_.value;
		}
	}

	if (!_land_pixels) {
		return ESP_OK;
	}

	// 2. Gezielt nur Landmassen "darüberstempeln"
	for (const auto &node : *_land_pixels) {
		if (node.y < frame.led_data.size() && node.x < frame.led_data[node.y].size()) {

			// Zurückwandeln in Float für fließende Farbübergänge
			float f_north = static_cast<float>(node.North) / 255.0F;
			float f_south = static_cast<float>(node.South) / 255.0F;
			float f_europe = static_cast<float>(node.Europe) / 255.0F;
			float f_africa = static_cast<float>(node.Africa) / 255.0F;
			float f_asia = static_cast<float>(node.Asia) / 255.0F;
			float f_australia = static_cast<float>(node.Australia) / 255.0F;

			float land_sum = f_north + f_south + f_europe + f_africa + f_asia + f_australia;
			float f_ocean = 1.0F - land_sum;
			f_ocean = std::max(f_ocean, 0.0F);

			RGB color = this->color_ocean_.value * f_ocean;
			color += (this->color_north_.value * f_north);
			color += (this->color_south_.value * f_south);
			color += (this->color_europe_.value * f_europe);
			color += (this->color_africa_.value * f_africa);
			color += (this->color_asia_.value * f_asia);
			color += (this->color_australia_.value * f_australia);

			frame.led_data[node.y][node.x] = color;
		}
	}

	return ESP_OK;
}

esp_err_t ContinentEffect::serialize() {
	JsonDocument doc;

	// Basisdaten
	doc["version"] = this->version_;
	doc["name"] = this->name_;
	doc["type"] = kType;

	// Parameter aufbauen
	JsonObject params = doc["parameters"].to<JsonObject>();
	JsonObject color = params["color"].to<JsonObject>();

	// Hilfsfunktion (Lambda), um den Code für die Farb-Arrays kurz zu halten
	auto add_color = [&](const char *key, const RGB &col) {
		JsonArray arr = color[key].to<JsonArray>();
		arr.add(col.red);
		arr.add(col.green);
		arr.add(col.blue);
	};

	add_color("north", this->color_north_.value);
	add_color("south", this->color_south_.value);
	add_color("europe", this->color_europe_.value);
	add_color("africa", this->color_africa_.value);
	add_color("asia", this->color_asia_.value);
	add_color("australia", this->color_australia_.value);
	add_color("ocean", this->color_ocean_.value);

	// Datei zum direkten Schreiben öffnen
	FILE *file = FileManager::open_file(this->path_, "wb");
	if (file == nullptr) { // KORRIGIERT: File-Check gefixt
		ESP_LOGE(Effect::kTag, "Failed to open file for writing: %s", this->path_.c_str());
		return ESP_FAIL;
	}

	// Stream-Adapter nutzen: Kein riesiger String-Buffer im RAM nötig!
	PosixFileWriter writer(file);
	serializeJson(doc, writer);
	FileManager::close_file(file);

	return ESP_OK;
}

esp_err_t ContinentEffect::deserialize(std::string path) {
	// 1. Datei streamend öffnen
	FILE *file = FileManager::open_file(path, "rb");
	ESP_LOGI(Effect::kTag, "Read file for path: %s", path.c_str());
	if (file == nullptr) {
		ESP_LOGW(Effect::kTag, "Read failed for path: %s", path.c_str());
		return ESP_FAIL;
	}

	this->path_ = path;

	PosixFileReader reader(file);
	JsonDocument doc;

	// 2. Dokument direkt von der SD-Karte in den Parser saugen
	DeserializationError error = deserializeJson(doc, reader);
	FileManager::close_file(file);

	if (error) {
		ESP_LOGW(Effect::kTag, "JSON Parse failed: %s", error.c_str());
		return ESP_FAIL;
	}

	// 3. Basis-Daten auslesen (mit den bisherigen Werten als Fallback)
	this->version_ = doc["version"] | this->version_;
	this->name_ = doc["name"] | this->name_;

	// Typ prüfen
	const char *type_str = doc["type"];
	if (type_str != nullptr && strcmp(type_str, kType) != 0) {
		ESP_LOGW(Effect::kTag, "Type mismatch! expected: %s received: %s", kType, type_str);
		return ESP_FAIL;
	}

	// 4. Farben sicher auslesen
	JsonObject color = doc["parameters"]["color"];
	if (!color.isNull()) {

		// Hilfsfunktion zum sauberen Einlesen der Arrays
		auto read_color = [&](const char *key, auto &color_obj) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			JsonArray arr = color[key];
			if (arr.size() >= 3) {
				color_obj.value.red = arr[0] | color_obj.value.red;
				color_obj.value.green = arr[1] | color_obj.value.green;
				color_obj.value.blue = arr[2] | color_obj.value.blue;
			}
		};

		read_color("north", this->color_north_);
		read_color("south", this->color_south_);
		read_color("europe", this->color_europe_);
		read_color("africa", this->color_africa_);
		read_color("asia", this->color_asia_);
		read_color("australia", this->color_australia_);
		read_color("ocean", this->color_ocean_);
	}
	ESP_LOGI(Effect::kTag, "Deserialized!");
	ESP_LOGI(Effect::kTag, "Color North: R=%d G=%d B=%d", this->color_north_.value.red, this->color_north_.value.green,
			 this->color_north_.value.blue);
	ESP_LOGI(Effect::kTag, "Color South: R=%d G=%d B=%d", this->color_south_.value.red, this->color_south_.value.green,
			 this->color_south_.value.blue);
	ESP_LOGI(Effect::kTag, "Color Europe: R=%d G=%d B=%d", this->color_europe_.value.red, this->color_europe_.value.green,
			 this->color_europe_.value.blue);
	ESP_LOGI(Effect::kTag, "Color Africa: R=%d G=%d B=%d", this->color_africa_.value.red, this->color_africa_.value.green,
			 this->color_africa_.value.blue);
	ESP_LOGI(Effect::kTag, "Color Asia: R=%d G=%d B=%d", this->color_asia_.value.red, this->color_asia_.value.green, this->color_asia_.value.blue);
	ESP_LOGI(Effect::kTag, "Color Australia: R=%d G=%d B=%d", this->color_australia_.value.red, this->color_australia_.value.green,
			 this->color_australia_.value.blue);
	ESP_LOGI(Effect::kTag, "Color Ocean: R=%d G=%d B=%d", this->color_ocean_.value.red, this->color_ocean_.value.green,
			 this->color_ocean_.value.blue);

	return ESP_OK;
}

esp_err_t ContinentEffect::set_parameter(const char *name, const char *value) {
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
	} else if (strcmp(name, "color_north") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_north_.value = RGB{.red = red, .green = green, .blue = blue};
	} else if (strcmp(name, "color_south") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_south_.value = RGB{.red = red, .green = green, .blue = blue};
	} else if (strcmp(name, "color_europe") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_europe_.value = RGB{.red = red, .green = green, .blue = blue};
	} else if (strcmp(name, "color_africa") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_africa_.value = RGB{.red = red, .green = green, .blue = blue};
	} else if (strcmp(name, "color_australia") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_australia_.value = RGB{.red = red, .green = green, .blue = blue};
	} else if (strcmp(name, "color_ocean") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_ocean_.value = RGB{.red = red, .green = green, .blue = blue};
	} else if (strcmp(name, "color_asia") == 0) {
		uint8_t red = 0U;
		uint8_t green = 0U;
		uint8_t blue = 0U;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		if (sscanf(value, "%" SCNu8 ",%" SCNu8 ",%" SCNu8, &red, &green, &blue) != 3) {
			return ESP_ERR_INVALID_ARG;
		}
		color_asia_.value = RGB{.red = red, .green = green, .blue = blue};
	} else {
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

esp_err_t ContinentEffect::set_filepath(std::string path) {
	this->path_ = path;
	return ESP_OK;
}

std::string ContinentEffect::get_filepath() { return this->path_; }

std::string ContinentEffect::get_name() { return this->name_; }