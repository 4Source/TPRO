#include "timeline_effect.hpp"
#include "effect_factory.hpp" // Needed in initialize_subeffects
#include <algorithm>
#include <cctype>
#include <esp_log.h>

std::unique_ptr<LedFrame> TimelineEffect::get_led_data(DateTime time) {
	if (steps_.empty()) {
		return std::make_unique<LedFrame>();
	}

	// Gesamt Dauer - wsh besser wenn von deserialize gesetzt wird
	total_duration_ms_ = 0;
	for (const auto &step : steps_) {
		total_duration_ms_ += step.duration_ms;
	}
	// Rechne aktuelle sekunde in ms - %gesamtloop um aktuelle position in timeline zu bekommen
	// Sollten vielleciht DateTime noch ein ms feld geben
	uint32_t current_ms = ((time.second * 1000) + 0 /*Mögliches DateTime.millisecond Feld*/) % total_duration_ms_;

	// aktiven Effekt finden
	uint32_t accumulated_ms = 0;
	for (const auto &step : steps_) {
		if (current_ms >= accumulated_ms && current_ms < (accumulated_ms + step.duration_ms)) {
			// zeit an subeffekt weiterrecihen
			return step.effect->get_led_data(time);
		}
		accumulated_ms += step.duration_ms;
	}
	// Ist timeline loop inkorrekt konfiguriert landen wir hier - eigentlich fehlerfall
	return steps_.front().effect->get_led_data(time);
}

esp_err_t TimelineEffect::set_parameter(const char *name, const char *value) { return ESP_OK; }
esp_err_t TimelineEffect::set_filepath(std::string path) {
	this->path_ = path;
	return ESP_OK;
}
std::string TimelineEffect::get_filepath() { return this->path_; }

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

esp_err_t TimelineEffect::initialize_subeffects() {
	// Clear existing
	this->subeffects_.clear();
	this->steps_.clear();

	this->subeffects_.reserve(this->subeffect_configs_.size());
	this->steps_.reserve(this->subeffect_configs_.size());

	uint32_t current_accumulated_ms = 0;

	for (size_t i = 0; i < this->subeffect_configs_.size(); ++i) {
		const auto &config = this->subeffect_configs_[i];

		auto effect = EffectFactory::generate_from_json(config.path);

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
		// Effekt gehört step und subeffects_ via shared_ptr
		this->subeffects_.push_back(effect);

		TimelineStep step;
		step.effect = effect;
		step.duration_ms = duration;
		step.start_ms = current_accumulated_ms;

		this->steps_.push_back(std::move(step));

		current_accumulated_ms += duration;
	}

	this->total_duration_ms_ = current_accumulated_ms;
	return ESP_OK;
}

// -----------------
// JSON
// -----------------

esp_err_t TimelineEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);
	path_ = std::move(path);
	if (!opt_json) {
		return ESP_FAIL;
	}
	// Standard fields
	esp_err_t err = EffectParser::parse_with_defaults(opt_json.value().c_str(), path_buffer_.data(), path_buffer_.size(),
													  [this](jparse_ctx_t *jctx) { return this->parse_static_fields(jctx); });

	if (err != ESP_OK) {
		return err;
	}
	// Key Value overwrites parsen
	if (err != ESP_OK) {
		return err;
	}

	// Create subeffects and overwrite parameter
	return this->initialize_subeffects();
}

esp_err_t TimelineEffect::parse_static_fields(jparse_ctx_t *jctx) {
	std::array<char, 64> name_buf{};
	if (json_obj_get_string(jctx, "name", name_buf.data(), name_buf.size()) == 0) {
		this->name_ = name_buf.data();
	}

	int temp_id = 0;
	int temp_cycle = 0;
	if (json_obj_get_int(jctx, "effect.parameters.primary.id", &temp_id) == 0) {
		this->primary_id_ = static_cast<uint32_t>(temp_id);
	}
	if (json_obj_get_int(jctx, "effect.parameters.primary.cycle_time", &temp_cycle) == 0) {
		this->primary_cycle_time_ = static_cast<uint32_t>(temp_cycle);
	}

	return ESP_OK;
}

esp_err_t TimelineEffect::parse_dynamic_secondary(const char *raw_json) {
	EffectParser::cJSON_ptr root(cJSON_Parse(raw_json), cJSON_Delete);
	if (root == nullptr) {
		return ESP_FAIL;
	}

	cJSON *effect = cJSON_GetObjectItem(root.get(), "effect");
	cJSON *params = (effect != nullptr) ? cJSON_GetObjectItem(effect, "parameters") : nullptr;

	// parse secondary items
	cJSON *secondary = (params != nullptr) ? cJSON_GetObjectItem(params, "secondary") : nullptr;

	if ((secondary != nullptr) && (cJSON_IsArray(secondary) != 0)) {
		int num_sec = cJSON_GetArraySize(secondary);
		this->secondary_items_.clear();
		this->secondary_items_.reserve(num_sec);

		cJSON *sec_item = nullptr;
		cJSON_ArrayForEach(sec_item, secondary) {
			TimelineSecondaryItem item{};

			cJSON *id_item = cJSON_GetObjectItem(sec_item, "id");
			cJSON *cycle_item = cJSON_GetObjectItem(sec_item, "cycle_time");
			item.id = (id_item != nullptr) ? static_cast<uint32_t>(id_item->valueint) : 0;
			item.cycle_time = (cycle_item != nullptr) ? static_cast<uint32_t>(cycle_item->valueint) : 0;

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
		}
	}

	// aubeffect array parsen
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

esp_err_t TimelineEffect::serialize(const std::string path) {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", "1.0");
	cJSON_AddStringToObject(root.get(), "name", this->name_.c_str());

	cJSON *effect = cJSON_AddObjectToObject(root.get(), "effect");
	cJSON_AddStringToObject(effect, "type", "timeline");
	cJSON *params = cJSON_AddObjectToObject(effect, "parameters");

	// primary
	cJSON *primary = cJSON_AddObjectToObject(params, "primary");
	cJSON_AddNumberToObject(primary, "id", this->primary_id_);
	cJSON_AddNumberToObject(primary, "cycle_time", this->primary_cycle_time_);
	cJSON_AddObjectToObject(primary, "overwrite");

	// secondaries
	cJSON *secondary = cJSON_AddArrayToObject(params, "secondary");
	for (const auto &sec_item : this->secondary_items_) {
		cJSON *sec_obj = cJSON_CreateObject();
		cJSON_AddNumberToObject(sec_obj, "id", sec_item.id);
		cJSON_AddNumberToObject(sec_obj, "cycle_time", sec_item.cycle_time);

		cJSON *overwrite = cJSON_AddObjectToObject(sec_obj, "overwrite");

		// N beliebige Overwrites
		for (const auto &pair : sec_item.overwrites) {
			// Ist string eine reine zahl - dann als Zahl ins JSON schreiben sonst als string
			bool is_number =
				!pair.second.empty() && std::ranges::all_of(pair.second, [](unsigned char character) { return std::isdigit(character); });

			if (is_number) {
				// write zahl
				cJSON_AddNumberToObject(overwrite, pair.first.c_str(), std::stoi(pair.second));
			} else {
				// write string
				cJSON_AddStringToObject(overwrite, pair.first.c_str(), pair.second.c_str());
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
	return FileManager::save_file(path, json_string.get());
}
