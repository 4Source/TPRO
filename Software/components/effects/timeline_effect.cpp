#include "effect/timeline_effect.hpp"
#include "effect_factory.hpp" // Needed in initialize_subeffects
#include <algorithm>
#include <cctype>
#include <esp_log.h>

esp_err_t TimelineEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	// Interrupt on Timestamp Logik
	for (const auto &item : secondary_items_) {
		if (item.is_active(time_stamp)) {
			if (item.id < subeffects_.size() && subeffects_[item.id] != nullptr) {
				auto &effect = subeffects_[item.id];

				// Logging nur bei Wechsel
				std::string log_name = "Interrupt: " + effect->get_name();
				if (this->current_active_name_ != log_name) {
					this->current_active_name_ = log_name;
					ESP_LOGD(Effect::kTag, "Timeline interrupted by ID %u: %s", item.id, effect->get_name().c_str());
				}

				// Wie lange läuft dieser spezifische Interrupt schon?
				uint32_t elapsed_ms = item.get_elapsed_ms(time_stamp);

				// Wir mappen die elapsed Zeit auf die definierte cycle_time des Effekts
				uint32_t effect_ms = (item.cycle_time > 0) ? (elapsed_ms % item.cycle_time) : elapsed_ms;

				// Relative Zeitstruktur für den Sub-Effekt aufbauen
				DateTime::TimeComponents rel_time = time_stamp;
				rel_time.hour = (effect_ms / 3600000);
				rel_time.minute = (effect_ms / 60000) % 60;
				rel_time.second = (effect_ms / 1000) % 60;
				rel_time.millisecond = effect_ms % 1000;

				// Rendern und beenden – die normale Timeline wird ignoriert
				return effect->get_led_data(frame, rel_time);
			}
		}
	}

	// Standard Logik
	if (steps_.empty()) {
		return ESP_ERR_INVALID_STATE;
	}
	if (total_duration_ms_ == 0) {
		return ESP_ERR_INVALID_STATE;
	}

	uint32_t total_ms = (time_stamp.hour * 3600000) + (time_stamp.minute * 60000) + (time_stamp.second * 1000) + time_stamp.millisecond;
	uint32_t current_ms = static_cast<uint32_t>((total_ms % total_duration_ms_) * speed_) % total_duration_ms_;
	uint32_t accumulated_ms = 0;

	for (const auto &step : this->steps_) {
		// Prüfen, ob current_ms in diesen Step fällt
		if (current_ms >= accumulated_ms && current_ms < (accumulated_ms + step.duration_ms)) {

			if (step.effect == nullptr) {
				return ESP_FAIL;
			}

			if (this->current_active_name_ != step.effect->get_name()) {
				this->current_active_name_ = step.effect->get_name();
				ESP_LOGD(Effect::kTag, "Switched to subeffect %s", this->current_active_name_.c_str());
			}

			// Relative Zeit für den Subeffekt berechnen
			uint32_t ms_into_step = current_ms - accumulated_ms;

			DateTime::TimeComponents relative_time = time_stamp;

			// Sauberer Zeit-Umbruch, falls Schritte sehr lang sind
			relative_time.minute = (ms_into_step / 60000) % 60;
			relative_time.second = (ms_into_step / 1000) % 60;
			relative_time.millisecond = (ms_into_step % 1000);

			return step.effect->get_led_data(frame, relative_time);
		}
		accumulated_ms += step.duration_ms;
	}

	ESP_LOGW(Effect::kTag, "TimelineEffect: No active step found for current_ms=%u (total_ms=%u)", current_ms, total_ms);

	// Letzter Fallback
	if (this->steps_.front().effect) {
		return this->steps_.front().effect->get_led_data(frame, time_stamp);
	}

	return ESP_FAIL;
}

esp_err_t TimelineEffect::set_parameter(const char *name, const char *value) { return ESP_OK; }

esp_err_t TimelineEffect::set_filepath(std::string path) {
	this->path_ = path;
	return ESP_OK;
}
std::string TimelineEffect::get_filepath() { return this->path_; }

std::string TimelineEffect::get_name() { return this->name_; }

esp_err_t TimelineEffect::set_subeffect(const std::shared_ptr<Effect> &effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}
	this->subeffects_.push_back(effect);
	return ESP_OK;
}

esp_err_t TimelineEffect::delete_subeffect(Effect *effect) {
	if (effect == nullptr) {
		return ESP_FAIL;
	}

	auto iterator = std::ranges::find(this->subeffects_, effect, &std::shared_ptr<Effect>::get);

	if (iterator != this->subeffects_.end()) {
		this->subeffects_.erase(iterator);
		return ESP_OK;
	}
	return ESP_FAIL; // Effekt wurde nicht gefunden
}
const std::vector<std::shared_ptr<Effect>> &TimelineEffect::get_subeffects() const { return this->subeffects_; }

/**
 * Added random type for timeline subeffects - if type or path is "random", a random effect from the SD card will be chosen
	{
  "type": "random",
  "path": "random"
}
*/
esp_err_t TimelineEffect::initialize_subeffects() {
	// Clear existing
	this->subeffects_.clear();
	this->steps_.clear();

	this->subeffects_.reserve(this->subeffect_configs_.size());
	this->steps_.reserve(this->subeffect_configs_.size());

	uint32_t current_accumulated_ms = 0;

	for (size_t i = 0; i < this->subeffect_configs_.size(); ++i) {
		const auto &config = this->subeffect_configs_[i];

		std::shared_ptr<Effect> effect = nullptr;

		if (config.path == "random" || config.type == "random") {
			auto random_path_opt = FileManager::get_random_effect_path();

			if (!random_path_opt.has_value()) {
				ESP_LOGE("TimelineEffect", "Konnte keinen zufälligen Effekt finden!");
				return ESP_FAIL;
			}

			effect = EffectFactory::generate_from_json(random_path_opt.value());
		} else {
			ESP_LOGD("TimelineEffect", "Initializing subeffect %zu with type '%s' and path '%s'", i, config.type.c_str(), config.path.c_str());
			effect = EffectFactory::generate_from_json(config.path);
		}

		if (effect == nullptr) {
			return ESP_FAIL;
		}

		uint32_t duration = 1000;

		// Add primary overwrites here
		if (i == this->primary_id_) {
			duration = this->primary_cycle_time_;
		} else {
			// Suche in secondary_items nach der passenden ID
			auto iterator = std::ranges::find_if(this->secondary_items_, [i](const TimelineSecondaryItem &item) { return item.id == i; });

			if (iterator != this->secondary_items_.end()) {
				duration = iterator->cycle_time;

				// Overwrites weitergeben
				for (const auto &[key, value] : iterator->overwrites) {
					if (effect->set_parameter(key.c_str(), value.c_str()) != ESP_OK) {
						ESP_LOGE("TimelineEffect", "Parameter '%s' konnte für Subeffect %zu nicht gesetzt werden", key.c_str(), i);
						return ESP_FAIL;
					}
				}
			}
		}
		// Effekt gehört step und subeffects via shared_ptr
		this->subeffects_.push_back(effect);

		TimelineStep step;
		step.effect = effect;
		step.duration_ms = duration;
		step.start_ms = current_accumulated_ms;

		this->steps_.push_back(std::move(step));

		current_accumulated_ms += duration;
	}

	this->total_duration_ms_ = current_accumulated_ms;
	ESP_LOGD(Effect::kTag, "Initialized %zu subeffects with total duration %lu ms", this->subeffects_.size(), this->total_duration_ms_);
	return ESP_OK;
}

// ---------------------------------------------------------
// JSON DESERIALIZATION & SETUP
// ---------------------------------------------------------

esp_err_t TimelineEffect::deserialize(std::string path) {
	ESP_LOGD(Effect::kTag, "Starting deserialization for path: %s", path.c_str());
	auto opt_json = FileManager::read_file(path);

	if (!opt_json) {
		ESP_LOGE(Effect::kTag, "Read failed: File could not be opened or is empty.");
		return ESP_FAIL;
	}

	this->path_ = path;

	// 1. Statische Felder parsen (Stream-Parser für einfache Werte)
	ESP_LOGD(Effect::kTag, "Parsing static fields...");
	esp_err_t err = EffectParser::parse_with_defaults(opt_json.value().c_str(), path_buffer_.data(), path_buffer_.size(),
													  [this](jparse_ctx_t *jctx) { return this->parse_static_fields(jctx); });

	if (err != ESP_OK) {
		ESP_LOGE(Effect::kTag, "Failed to parse static fields. Error code: %d", err);
		return err;
	}

	// 2. Dynamische Arrays parsen (DOM-Parser für komplexe Strukturen)
	ESP_LOGD(Effect::kTag, "Parsing dynamic secondary fields...");
	err = this->parse_dynamic_secondary(opt_json.value().c_str());

	if (err != ESP_OK) {
		ESP_LOGE(Effect::kTag, "Failed to parse dynamic secondary fields. Error code: %d", err);
		return err;
	}

	// 3. Subeffekte anhand der geparsten Configs initialisieren
	ESP_LOGD(Effect::kTag, "Deserialization successful. Initializing subeffects...");
	err = this->initialize_subeffects();
	if (err != ESP_OK) {
		ESP_LOGE(Effect::kTag, "Failed to initialize subeffects. Error code: %d", err);
		return err;
	}

	// 4. Timeline-Schritte final aufbauen
	ESP_LOGD(Effect::kTag, "Building timeline steps...");
	return this->build_timeline_steps();
}

esp_err_t TimelineEffect::parse_static_fields(jparse_ctx_t *jctx) {
	std::array<char, 64> str_buf{};

	if (json_obj_get_string(jctx, "name", str_buf.data(), str_buf.size()) == 0) {
		this->name_ = std::string(str_buf.data());
		ESP_LOGD(Effect::kTag, "Parsed name: %s", this->name_.c_str());
	}

	if (json_obj_get_string(jctx, "version", str_buf.data(), str_buf.size()) == 0) {
		this->version_ = std::string(str_buf.data());
	}

	// Parameters -> Primary
	if (json_obj_get_object(jctx, "parameters") == 0) {
		if (json_obj_get_object(jctx, "primary") == 0) {
			int temp_val = 0;

			if (json_obj_get_int(jctx, "id", &temp_val) == 0) {
				this->primary_id_ = static_cast<uint32_t>(temp_val);
				ESP_LOGD(Effect::kTag, "Parsed primary.id: %lu", this->primary_id_);
			} else {
				ESP_LOGW(Effect::kTag, "id not found in primary");
			}

			if (json_obj_get_int(jctx, "cycle_time", &temp_val) == 0) {
				this->primary_cycle_time_ = static_cast<uint32_t>(temp_val);
				ESP_LOGD(Effect::kTag, "Parsed primary.cycle_time: %lu", this->primary_cycle_time_);
			} else {
				ESP_LOGW(Effect::kTag, "cycle_time not found in primary");
			}

			json_obj_leave_object(jctx); // Leave 'primary'
		} else {
			ESP_LOGW(Effect::kTag, "parameters.primary object not found");
		}
		json_obj_leave_object(jctx); // Leave 'parameters'
	} else {
		ESP_LOGW(Effect::kTag, "parameters object not found");
	}

	return ESP_OK;
}

esp_err_t TimelineEffect::parse_dynamic_secondary(const char *raw_json) {
	EffectParser::cJSON_ptr root(cJSON_Parse(raw_json), cJSON_Delete);
	if (root == nullptr) {
		ESP_LOGE(Effect::kTag, "cJSON_Parse failed. Invalid JSON structure.");
		return ESP_FAIL;
	}

	cJSON *params = cJSON_GetObjectItem(root.get(), "parameters");
	if (params == nullptr) {
		ESP_LOGW(Effect::kTag, "No 'parameters' object found in root.");
	}

	// --- SECONDARY ITEMS PARSEN ---
	cJSON *secondary = (params != nullptr) ? cJSON_GetObjectItem(params, "secondary") : nullptr;

	if ((secondary != nullptr) && (cJSON_IsArray(secondary) != 0)) {
		int num_sec = cJSON_GetArraySize(secondary);
		ESP_LOGD(Effect::kTag, "Found 'secondary' array with %d items.", num_sec);

		this->secondary_items_.clear();
		this->secondary_items_.reserve(num_sec);

		cJSON *sec_item = nullptr;
		int idx = 0;
		cJSON_ArrayForEach(sec_item, secondary) {
			TimelineSecondaryItem item{};

			cJSON *id_item = cJSON_GetObjectItem(sec_item, "id");
			cJSON *cycle_item = cJSON_GetObjectItem(sec_item, "cycle_time");
			cJSON *interrupt_item = cJSON_GetObjectItem(sec_item, "interrupt_time");
			cJSON *datetime_item = cJSON_GetObjectItem(sec_item, "on_datetime");

			item.id = (id_item != nullptr) ? static_cast<uint32_t>(id_item->valueint) : 0;
			item.cycle_time = (cycle_item != nullptr) ? static_cast<uint32_t>(cycle_item->valueint) : 0;
			item.interrupt_time = (interrupt_item != nullptr) ? static_cast<uint32_t>(interrupt_item->valueint) : 0;

			// Zeitstempel für Interrupts (MM:DD:HH:mm)
			if (datetime_item != nullptr && cJSON_IsString(datetime_item) != 0) {
				int month = 0; // 0 -> jeden Monat
				int day = 0;   // 0 -> jeden Tag
				int hour = -1; // -1 -> jede Stunde
				int min = 0;
				// %d erlaubt das Parsen von -1 für die Stunden-Wildcard
				// NOLINTNEXTLINE[cppcoreguidelines-pro-type-vararg]
				if (sscanf(datetime_item->valuestring, "%d:%d:%d:%d", &month, &day, &hour, &min) == 4) {
					item.trigger.month = static_cast<uint8_t>(month);
					item.trigger.day = static_cast<uint8_t>(day);
					item.trigger.hour = static_cast<int8_t>(hour);
					item.trigger.minute = static_cast<uint8_t>(min);
				} else {
					ESP_LOGW(Effect::kTag, "Invalid on_datetime format: %s", datetime_item->valuestring);
				}
			}

			// Parameter Overwrites
			cJSON *overwrite_obj = cJSON_GetObjectItem(sec_item, "overwrite");
			if ((overwrite_obj != nullptr) && (cJSON_IsObject(overwrite_obj) != 0)) {
				cJSON *ow_item = nullptr;
				cJSON_ArrayForEach(ow_item, overwrite_obj) {
					if (ow_item->string != nullptr) {
						std::string key = ow_item->string;
						std::string value;

						if (cJSON_IsString(ow_item) != 0) {
							value = ow_item->valuestring;
						} else if (cJSON_IsNumber(ow_item) != 0) {
							value = std::to_string(ow_item->valueint);
						}
						item.overwrites[key] = value;
					}
				}
			}
			this->secondary_items_.push_back(item);
			idx++;
		}
	}

	// --- SUBEFFECT CONFIGS PARSEN ---
	cJSON *subeffects = cJSON_GetObjectItem(root.get(), "subeffects");

	if ((subeffects != nullptr) && (cJSON_IsArray(subeffects) != 0)) {
		int num_sub = cJSON_GetArraySize(subeffects);

		this->subeffect_configs_.clear();
		this->subeffect_configs_.reserve(num_sub);

		cJSON *sub_item = nullptr;
		cJSON_ArrayForEach(sub_item, subeffects) {
			TimelineSubeffectConfig conf{};

			cJSON *type_item = cJSON_GetObjectItem(sub_item, "type");
			cJSON *path_item = cJSON_GetObjectItem(sub_item, "path");

			if ((type_item != nullptr) && (cJSON_IsString(type_item) != 0)) {
				conf.type = type_item->valuestring;
			}
			if ((path_item != nullptr) && (cJSON_IsString(path_item) != 0)) {
				conf.path = path_item->valuestring;
			}

			this->subeffect_configs_.push_back(conf);
		}
	}

	return ESP_OK;
}

esp_err_t TimelineEffect::build_timeline_steps() {
	this->steps_.clear();
	this->total_duration_ms_ = 0;

	for (size_t i = 0; i < this->secondary_items_.size(); i++) {

		// Sicherstellen, dass der korrespondierende Subeffekt existiert
		if (i < this->subeffects_.size() && this->subeffects_[i] != nullptr) {
			TimelineStep new_step;

			new_step.effect = this->subeffects_[i];
			new_step.duration_ms = this->secondary_items_[i].cycle_time;
			new_step.id = this->secondary_items_[i].id;

			// Startzeitpunkt merken (akkumuliert)
			new_step.start_ms = this->total_duration_ms_;

			this->steps_.push_back(new_step);
			this->total_duration_ms_ += new_step.duration_ms;

			ESP_LOGD(Effect::kTag, "Step %zu verknüpft: ID %lu, Duration %lu ms", i, new_step.id, new_step.duration_ms);
		} else {
			ESP_LOGW(Effect::kTag, "Fehler bei Step %zu: Kein zugehöriger Subeffekt gefunden oder Subeffekt ist NULL.", i);
		}
	}

	if (this->steps_.empty() || this->total_duration_ms_ == 0) {
		ESP_LOGE(Effect::kTag, "Konnte keine gültigen Steps aufbauen oder total_duration_ms ist 0!");
		return ESP_FAIL;
	}

	ESP_LOGD(Effect::kTag, "Timeline erfolgreich aufgebaut. Total duration: %lu ms", this->total_duration_ms_);
	return ESP_OK;
}

esp_err_t TimelineEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", this->version_.c_str());
	cJSON_AddStringToObject(root.get(), "name", this->name_.c_str());
	cJSON_AddStringToObject(root.get(), "type", kType);
	cJSON *params = cJSON_AddObjectToObject(root.get(), "parameters");

	// ---------------------------------------------------------
	// primary
	// ---------------------------------------------------------
	cJSON *primary = cJSON_AddObjectToObject(params, "primary");
	cJSON_AddNumberToObject(primary, "id", this->primary_id_);
	cJSON_AddNumberToObject(primary, "cycle_time", this->primary_cycle_time_);

	cJSON *primary_overwrite = cJSON_AddObjectToObject(primary, "overwrite");

	// N beliebige Overwrites für Primary
	for (const auto &pair : this->overwrites_) {
		// Ist string eine reine Zahl - dann als Zahl ins JSON schreiben sonst als string
		bool is_number = !pair.second.empty() && std::ranges::all_of(pair.second, [](unsigned char character) { return std::isdigit(character); });

		if (is_number) {
			cJSON_AddNumberToObject(primary_overwrite, pair.first.c_str(), std::stoi(pair.second));
		} else {
			cJSON_AddStringToObject(primary_overwrite, pair.first.c_str(), pair.second.c_str());
		}
	}

	// ---------------------------------------------------------
	// secondaries
	// ---------------------------------------------------------
	cJSON *secondary = cJSON_AddArrayToObject(params, "secondary");
	for (const auto &sec_item : this->secondary_items_) {
		cJSON *sec_obj = cJSON_CreateObject();
		cJSON_AddNumberToObject(sec_obj, "id", sec_item.id);
		cJSON_AddNumberToObject(sec_obj, "cycle_time", sec_item.cycle_time);

		// interrupt_time und on_datetime
		cJSON_AddNumberToObject(sec_obj, "interrupt_time", sec_item.interrupt_time);

		std::array<char, 64> datetime_buf{};
		// NOLINTNEXTLINE[cppcoreguidelines-pro-type-vararg]
		std::snprintf(datetime_buf.data(), datetime_buf.size(), "%02u:%02u:%d:%02u", sec_item.trigger.month, sec_item.trigger.day,
					  static_cast<int>(sec_item.trigger.hour), sec_item.trigger.minute);
		cJSON_AddStringToObject(sec_obj, "on_datetime", datetime_buf.data());

		cJSON *secondary_overwrite = cJSON_AddObjectToObject(sec_obj, "overwrite");

		// N beliebige Overwrites für Secondary
		for (const auto &pair : sec_item.overwrites) {
			bool is_number =
				!pair.second.empty() && std::ranges::all_of(pair.second, [](unsigned char character) { return std::isdigit(character); });

			if (is_number) {
				cJSON_AddNumberToObject(secondary_overwrite, pair.first.c_str(), std::stoi(pair.second));
			} else {
				cJSON_AddStringToObject(secondary_overwrite, pair.first.c_str(), pair.second.c_str());
			}
		}

		cJSON_AddItemToArray(secondary, sec_obj);
	}

	// ---------------------------------------------------------
	// subeffects
	// ---------------------------------------------------------
	cJSON *subeffects = cJSON_AddArrayToObject(root.get(), "subeffects");
	for (const auto &sub_conf : this->subeffect_configs_) {
		cJSON *sub_obj = cJSON_CreateObject();
		cJSON_AddStringToObject(sub_obj, "type", sub_conf.type.c_str());
		cJSON_AddStringToObject(sub_obj, "path", sub_conf.path.c_str());
		cJSON_AddItemToArray(subeffects, sub_obj);
	}

	// ---------------------------------------------------------
	// SCHEMA
	// ---------------------------------------------------------
	cJSON *schema = cJSON_AddObjectToObject(root.get(), "schema");

	// Helfer-Lambda für Zahlen- und Datetime-Schemas
	auto add_schema_prop = [&](cJSON *parent, const char *key, const char *type, int min = 0, int max = 0, int step = 0) {
		cJSON *obj = cJSON_AddObjectToObject(parent, key);
		cJSON_AddStringToObject(obj, "type", type);

		if (std::strcmp(type, "number") == 0 || std::strcmp(type, "range") == 0) {
			cJSON_AddNumberToObject(obj, "min", min);
			cJSON_AddNumberToObject(obj, "max", max);
			cJSON_AddNumberToObject(obj, "step", step);
		}
	};

	// Primary Schema
	cJSON *primary_schema = cJSON_AddObjectToObject(schema, "primary");
	add_schema_prop(primary_schema, "id", "number", 0, 9999, 1);
	add_schema_prop(primary_schema, "cycle_time", "number", 0, 3600000, 100);

	// Secondary Array Schema
	cJSON *secondary_schema = cJSON_AddObjectToObject(schema, "secondary");
	cJSON_AddStringToObject(secondary_schema, "type", "array");

	// "items" definiert, wie EIN Objekt im "secondary" Array aussieht
	cJSON *sec_items_schema = cJSON_AddObjectToObject(secondary_schema, "items");

	add_schema_prop(sec_items_schema, "id", "number", 0, 9999, 1);
	add_schema_prop(sec_items_schema, "cycle_time", "number", 0, 3600000, 100);
	add_schema_prop(sec_items_schema, "interrupt_time", "number", 0, 3600000, 1000);
	add_schema_prop(sec_items_schema, "on_datetime", "datetime");

	EffectParser::cJSON_str_ptr json_string(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(this->path_, json_string.get());
}
