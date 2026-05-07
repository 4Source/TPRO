#include "effect/timeline_effect.hpp"
#include "effect_factory.hpp" // Needed in initialize_subeffects
#include <algorithm>
#include <cctype>
#include <esp_log.h>

esp_err_t TimelineEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}
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
				ESP_LOGI(Effect::kTag, "Switched to subeffect %s", this->current_active_name_.c_str());
			}

			// Relative Zeit für den Subeffekt berechnen
			uint32_t ms_into_step = current_ms - accumulated_ms;

			DateTime::TimeComponents relative_time = time_stamp; // Wieder time_stamp nutzen

			// Sauberer Zeit-Umbruch, falls Schritte sehr lang sind
			relative_time.minute = (ms_into_step / 60000) % 60;
			relative_time.second = (ms_into_step / 1000) % 60;
			relative_time.millisecond = (ms_into_step % 1000);

			// frame an den Subeffekt weitergeben!
			return step.effect->get_led_data(frame, relative_time);
		}
		accumulated_ms += step.duration_ms;
	}

	ESP_LOGW(Effect::kTag, "TimelineEffect: No active step found for current_ms=%u (total_ms=%u)", current_ms, total_ms);

	// Fallback
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
			ESP_LOGI("TimelineEffect", "Initializing subeffect %zu with type '%s' and path '%s'", i, config.type.c_str(), config.path.c_str());
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
	ESP_LOGI(Effect::kTag, "Initialized %zu subeffects with total duration %lu ms", this->subeffects_.size(), this->total_duration_ms_);
	return ESP_OK;
}

// -----------------
// JSON
// -----------------

esp_err_t TimelineEffect::deserialize(std::string path) {
	ESP_LOGI(Effect::kTag, "Starting deserialization for path: %s", path.c_str());
	auto opt_json = FileManager::read_file(path);

	if (!opt_json) {
		ESP_LOGE(Effect::kTag, "Read failed: File could not be opened or is empty.");
		return ESP_FAIL;
	}

	this->path_ = path;

	// Standard fields
	ESP_LOGI(Effect::kTag, "Parsing static fields...");
	esp_err_t err = EffectParser::parse_with_defaults(opt_json.value().c_str(), path_buffer_.data(), path_buffer_.size(),
													  [this](jparse_ctx_t *jctx) { return this->parse_static_fields(jctx); });

	if (err != ESP_OK) {
		ESP_LOGE(Effect::kTag, "Failed to parse static fields. Error code: %d", err);
		return err;
	}

	// Subeffects parsen
	ESP_LOGI(Effect::kTag, "Parsing dynamic secondary fields...");
	err = this->parse_dynamic_secondary(opt_json.value().c_str());

	// Key Value overwrites parsen
	if (err != ESP_OK) {
		ESP_LOGE(Effect::kTag, "Failed to parse dynamic secondary fields. Error code: %d", err);
		return err;
	}

	ESP_LOGI(Effect::kTag, "Deserialization successful. Initializing subeffects...");
	// Create subeffects and overwrite parameter
	return this->initialize_subeffects();
}

esp_err_t TimelineEffect::parse_static_fields(jparse_ctx_t *jctx) {
	// ... [Previous code for version, name, type is correct] ...

	// 1. Enter the "parameters" object
	if (json_obj_get_object(jctx, "parameters") == 0) {

		// 2. Enter the "primary" object
		if (json_obj_get_object(jctx, "primary") == 0) {

			int temp_val = 0;

			// Now we are inside 'primary', so we access keys directly
			if (json_obj_get_int(jctx, "id", &temp_val) == 0) {
				this->primary_id_ = static_cast<uint32_t>(temp_val);
				ESP_LOGI(Effect::kTag, "Parsed primary.id: %lu", this->primary_id_);
			} else {
				ESP_LOGW(Effect::kTag, "id not found in primary");
			}

			if (json_obj_get_int(jctx, "cycle_time", &temp_val) == 0) {
				this->primary_cycle_time_ = static_cast<uint32_t>(temp_val);
				ESP_LOGI(Effect::kTag, "Parsed primary.cycle_time: %lu", this->primary_cycle_time_);
			} else {
				ESP_LOGW(Effect::kTag, "cycle_time not found in primary");
			}

			// 3. Leave the "primary" object
			json_obj_leave_object(jctx);
		} else {
			ESP_LOGW(Effect::kTag, "parameters.primary object not found");
		}

		// 4. Leave the "parameters" object
		json_obj_leave_object(jctx);
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

	// BUGFIX: cJSON_GetObjectItem verwenden statt cJSON_AddObjectToObject!
	cJSON *params = cJSON_GetObjectItem(root.get(), "parameters");
	if (params == nullptr) {
		ESP_LOGW(Effect::kTag, "No 'parameters' object found in root.");
	}

	// parse secondary items
	cJSON *secondary = (params != nullptr) ? cJSON_GetObjectItem(params, "secondary") : nullptr;

	if ((secondary != nullptr) && (cJSON_IsArray(secondary) != 0)) {
		int num_sec = cJSON_GetArraySize(secondary);
		ESP_LOGI(Effect::kTag, "Found 'secondary' array with %d items.", num_sec);

		this->secondary_items_.clear();
		this->secondary_items_.reserve(num_sec);

		cJSON *sec_item = nullptr;
		int idx = 0;
		cJSON_ArrayForEach(sec_item, secondary) {
			TimelineSecondaryItem item{};

			cJSON *id_item = cJSON_GetObjectItem(sec_item, "id");
			cJSON *cycle_item = cJSON_GetObjectItem(sec_item, "cycle_time");
			item.id = (id_item != nullptr) ? static_cast<uint32_t>(id_item->valueint) : 0;
			item.cycle_time = (cycle_item != nullptr) ? static_cast<uint32_t>(cycle_item->valueint) : 0;

			ESP_LOGI(Effect::kTag, "  Secondary item [%d]: id=%lu, cycle_time=%lu", idx, item.id, item.cycle_time);

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
						ESP_LOGI(Effect::kTag, "    Overwrite found: %s = %s", key.c_str(), value.c_str());
					}
				}
			}
			this->secondary_items_.push_back(item);
			idx++;
		}
	} else {
		ESP_LOGW(Effect::kTag, "'secondary' array not found or is not an array.");
	}

	// subeffect array parsen
	cJSON *subeffects = cJSON_GetObjectItem(root.get(), "subeffects");

	if ((subeffects != nullptr) && (cJSON_IsArray(subeffects) != 0)) {
		int num_sub = cJSON_GetArraySize(subeffects);
		ESP_LOGI(Effect::kTag, "Found 'subeffects' array with %d items.", num_sub);

		this->subeffect_configs_.clear();
		this->subeffect_configs_.reserve(num_sub);

		cJSON *sub_item = nullptr;
		int sub_idx = 0;
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

			ESP_LOGI(Effect::kTag, "  Subeffect [%d]: type='%s', path='%s'", sub_idx, conf.type.c_str(), conf.path.c_str());

			this->subeffect_configs_.push_back(conf);
			sub_idx++;
		}
	} else {
		ESP_LOGW(Effect::kTag, "'subeffects' array not found or is not an array.");
	}
	// Nachdem ALLES geparst wurde:
	this->steps_.clear();
	this->total_duration_ms_ = 0;

	// Wir gehen durch die secondary_items, da diese die "Dauer" bestimmen
	for (size_t i = 0; i < this->secondary_items_.size(); i++) {
		// Sicherstellen, dass wir einen passenden Subeffekt haben
		if (i < this->subeffects_.size()) {
			TimelineStep new_step;

			// 1. Die Logik (das Effekt-Objekt) zuweisen
			new_step.effect = this->subeffects_[i];

			// 2. Die Dauer aus dem geparsten secondary_item nehmen
			new_step.duration_ms = this->secondary_items_[i].cycle_time;

			// 3. Die ID zuweisen (wichtig für Overwrites/Zustände)
			new_step.id = this->secondary_items_[i].id;

			this->steps_.push_back(new_step);
			this->total_duration_ms_ += new_step.duration_ms;

			ESP_LOGI(Effect::kTag, "Step %zu verknüpft: ID %lu, Duration %lu ms", i, new_step.id, new_step.duration_ms);
		}
	}
	return ESP_OK;
}

esp_err_t TimelineEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", this->version_.c_str());
	cJSON_AddStringToObject(root.get(), "name", this->name_.c_str());
	cJSON_AddStringToObject(root.get(), "type", kType);
	cJSON *params = cJSON_AddObjectToObject(root.get(), "parameters");

	// primary
	cJSON *primary = cJSON_AddObjectToObject(params, "primary");
	cJSON_AddNumberToObject(primary, "id", this->primary_id_);
	cJSON_AddNumberToObject(primary, "cycle_time", this->primary_cycle_time_);
	cJSON *primary_overwrite = cJSON_AddObjectToObject(primary, "overwrite");

	// // N beliebige Overwrites
	// for (const auto &pair : this->primary_item.overwrites) {
	// 	// Ist string eine reine zahl - dann als Zahl ins JSON schreiben sonst als string
	// 	bool is_number = !pair.second.empty() && std::ranges::all_of(pair.second, [](unsigned char character) { return std::isdigit(character); });

	// 	if (is_number) {
	// 		// write zahl
	// 		cJSON_AddNumberToObject(primary_overwrite, pair.first.c_str(), std::stoi(pair.second));
	// 	} else {
	// 		// write string
	// 		cJSON_AddStringToObject(primary_overwrite, pair.first.c_str(), pair.second.c_str());
	// 	}
	// }

	// secondaries
	cJSON *secondary = cJSON_AddArrayToObject(params, "secondary");
	for (const auto &sec_item : this->secondary_items_) {
		cJSON *sec_obj = cJSON_CreateObject();
		cJSON_AddNumberToObject(sec_obj, "id", sec_item.id);
		cJSON_AddNumberToObject(sec_obj, "cycle_time", sec_item.cycle_time);

		cJSON *secondary_overwrite = cJSON_AddObjectToObject(sec_obj, "overwrite");

		// N beliebige Overwrites
		for (const auto &pair : sec_item.overwrites) {
			// Ist string eine reine zahl - dann als Zahl ins JSON schreiben sonst als string
			bool is_number =
				!pair.second.empty() && std::ranges::all_of(pair.second, [](unsigned char character) { return std::isdigit(character); });

			if (is_number) {
				// write zahl
				cJSON_AddNumberToObject(secondary_overwrite, pair.first.c_str(), std::stoi(pair.second));
			} else {
				// write string
				cJSON_AddStringToObject(secondary_overwrite, pair.first.c_str(), pair.second.c_str());
			}
		}

		cJSON_AddItemToArray(secondary, sec_obj);
	}

	// subeffects
	cJSON *subeffects = cJSON_AddArrayToObject(root.get(), "subeffects");
	for (const auto &sub_conf : this->subeffect_configs_) {
		cJSON *sub_obj = cJSON_CreateObject();
		cJSON_AddStringToObject(sub_obj, "type", sub_conf.type.c_str());
		cJSON_AddStringToObject(sub_obj, "path", sub_conf.path.c_str());
		cJSON_AddItemToArray(subeffects, sub_obj);
	}

	EffectParser::cJSON_str_ptr json_string(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(this->path_, json_string.get());
}
