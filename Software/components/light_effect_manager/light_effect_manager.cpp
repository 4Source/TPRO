#include "light_effect_manager.hpp"
#include "config_manager.hpp"
#include "datetime.hpp"
#include "led_frame.hpp"
#include "task_handles.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <esp_err.h>
#include <freertos/task.h>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static uint16_t log_counter = 0;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

LightEffectManager::LightEffectManager(LedFrame &external_frame, ConfigManager *config_manager)
	: data_(external_frame), config_manager_(config_manager) {}

// ============================================================
// Effektverwaltung
// ============================================================

esp_err_t LightEffectManager::set_effect(const std::shared_ptr<Effect> &effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	current_effect_ = effect;
	return ESP_OK;
}

esp_err_t LightEffectManager::set_effect(const std::string &path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	// Voraussetzung:
	// - Effekt wurde mit register_effect(...) registriert
	// - Effekt liefert sinnvollen Pfad über get_filepath()

	for (const std::shared_ptr<Effect> &effect : available_effects_) {
		if (effect == nullptr) {
			continue;
		}

		const std::string effect_path = effect->get_filepath();
		if (effect_path.empty()) {
			continue;
		}

		if (effect_path == path) {
			current_effect_ = effect;
			return ESP_OK;
		}
	}

	return ESP_ERR_NOT_FOUND;
}

esp_err_t LightEffectManager::register_effect(const std::shared_ptr<Effect> &effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	auto iterator = std::ranges::find(available_effects_, effect);
	if (iterator != available_effects_.end()) {
		ESP_LOGW(kTag, "Effect already registered: '%s' from '%s'", effect->get_name().c_str(), effect->get_filepath().c_str());
		return ESP_OK; // schon vorhanden
	}

	ESP_LOGD(kTag, "Registered effect: '%s' from '%s'", effect->get_name().c_str(), effect->get_filepath().c_str());

	available_effects_.push_back(effect);
	return ESP_OK;
}

esp_err_t LightEffectManager::unregister_effect(const std::shared_ptr<Effect> &effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	auto iterator = std::ranges::find(available_effects_, effect);
	if (iterator == available_effects_.end()) {
		return ESP_ERR_NOT_FOUND;
	}

	if (current_effect_ == effect) {
		current_effect_ = nullptr;
	}

	available_effects_.erase(iterator);
	return ESP_OK;
}

esp_err_t LightEffectManager::reload_effect(const std::string &path) {
	auto it = std::ranges::find_if(available_effects_, [&path](const auto &e) { return e && e->get_filepath() == path; });

	const bool was_current = (it != available_effects_.end() && *it == current_effect_);

	if (it != available_effects_.end()) {
		available_effects_.erase(it);
	}

	auto new_effect = EffectFactory::generate_from_json(path);
	if (!new_effect) {
		return ESP_FAIL;
	}

	available_effects_.push_back(new_effect);

	if (was_current) {
		current_effect_ = new_effect;
	}

	return ESP_OK;
}

std::shared_ptr<Effect> LightEffectManager::get_effect(const std::string &path) {
	auto iterator = std::ranges::find_if(available_effects_, [&path](const auto &effect) { return effect && effect->get_filepath() == path; });

	if (iterator != available_effects_.end()) {
		return *iterator;
	}
	// Gibt es noch nicht, versuche zu erzeugen
	auto new_effect = EffectFactory::generate_from_json(path);
	if (new_effect) {
		available_effects_.push_back(new_effect);
		return new_effect;
	}

	return nullptr;
}

// ============================================================
// Update-Logik
// ============================================================

bool parse_time(const std::string &time_string, uint8_t &hour, uint8_t &minute) {
	// Expected format: "HH:MM"
	if (time_string.size() != 5) {
		return false;
	}
	if (time_string[2] != ':') {
		return false;
	}

	if (time_string[0] < '0' || time_string[0] > '9') {
		return false;
	}
	if (time_string[1] < '0' || time_string[1] > '9') {
		return false;
	}
	if (time_string[3] < '0' || time_string[3] > '9') {
		return false;
	}
	if (time_string[4] < '0' || time_string[4] > '9') {
		return false;
	}

	uint8_t temp_hour = ((time_string[0] - '0') * 10) + (time_string[1] - '0');
	uint8_t temp_minute = ((time_string[3] - '0') * 10) + (time_string[4] - '0');

	if (temp_hour > 23 || temp_minute > 59) {
		return false;
	}

	hour = temp_hour;
	minute = temp_minute;
	return true;
}

bool LightEffectManager::isDurringWorkingHours(const DateTime::TimeComponents &time_stamp) {
	uint8_t time_from_hour = 0;
	uint8_t time_from_minute = 0;
	if (!parse_time(config_manager_->get_config("active_from"), time_from_hour, time_from_minute)) {
		ESP_LOGE(kTag, "Failed to parse 'active_from' to hours and minuets");
		return false;
	}

	uint8_t time_to_hour = 0;
	uint8_t time_to_minute = 0;
	if (!parse_time(config_manager_->get_config("active_to"), time_to_hour, time_to_minute)) {
		ESP_LOGE(kTag, "Failed to parse 'active_to' to hours and minuets");
		return false;
	}

	int from_total = (time_from_hour * 60) + time_from_minute;
	int to_total = (time_to_hour * 60) + time_to_minute;
	int current_total = (time_stamp.hour * 60) + time_stamp.minute;

	return from_total <= current_total && current_total <= to_total;
}

esp_err_t LightEffectManager::run() {
	auto time_stamp = DateTime::get_now();

	if (current_effect_ == nullptr && log_counter % 500 == 0) {
		ESP_LOGW(kTag, "current_effect is NULL!");
		return ESP_ERR_INVALID_STATE;
	}

	esp_err_t err = ESP_FAIL;
	if (isDurringWorkingHours(time_stamp)) {
		// Durring working hours get the frame from the effect
		err = current_effect_->get_led_data(this->working_frame_, time_stamp);
	} else {
		// Durring NONE working hours get frame with all off
		for (auto &led_row : this->working_frame_.led_data) {
			for (auto &led_pos : led_row) {
				led_pos = RGB{.red = 0, .green = 0, .blue = 0};
			}
		}
		err = ESP_OK;
	}

	if (err != ESP_OK) {
		ESP_LOGW(kTag, "Effect returned error code %d for timestamp %u-%02u-%02u %02u:%02u:%02u", err, time_stamp.year, time_stamp.month,
				 time_stamp.day, time_stamp.hour, time_stamp.minute, time_stamp.second);
		return err;
	}

	if (log_counter % 500 == 0) {
		ESP_LOGD(kTag, "Effect generated new frame for timestamp %u-%02u-%02u %02u:%02u:%02u", time_stamp.year, time_stamp.month, time_stamp.day,
				 time_stamp.hour, time_stamp.minute, time_stamp.second);
		ESP_LOGD(kTag, "New frame data_ (first LED): R=%d G=%d B=%d", this->working_frame_.led_data[0][0].red,
				 this->working_frame_.led_data[0][0].green, this->working_frame_.led_data[0][0].blue);
	}

	bool frame_changed = false;
	if (log_counter % 30 == 0) {
		frame_changed = true;
	}

	{
		LedFrame::ScopedWriteLock write_lock(data_);

		uint32_t hash_new = this->working_frame_.hash();
		uint32_t hash_old = data_.hash();

		if (hash_new != hash_old) {
			data_.copy_data_from(this->working_frame_);
			frame_changed = true;
		}
	}

	if (frame_changed) {
		if (TaskHandle::x_websocket_task_handle != nullptr) {
			xTaskNotifyGive(TaskHandle::x_websocket_task_handle);
		}
		if (TaskHandle::x_led_controller_task_handle != nullptr) {
			xTaskNotifyGive(TaskHandle::x_led_controller_task_handle);
		}
	}

	return ESP_OK;
}

// ============================================================
// System-Anbindung & Konfiguration
// ============================================================

void LightEffectManager::set_config_manager(ConfigManager *manager) {
	if (config_manager_ != nullptr) {
		if (config_manager_->remove_observer("current_effect", *this) != ESP_OK) {
			ESP_LOGE(kTag, "Failed to remove observer in old config manager");
		}
	}

	config_manager_ = manager;

	if (config_manager_ != nullptr) {
		if (config_manager_->add_observer("current_effect", *this) != ESP_OK) {
			ESP_LOGE(kTag, "Failed to add observer in new config manager");
		}
	}
}

void LightEffectManager::update(const std::string &key) {
	if (config_manager_ == nullptr) {
		ESP_LOGW(kTag, "update() called but config_manager_ is null");
		return;
	}

	if (key == "current_effect") {
		auto path = config_manager_->get_config("current_effect");
		ESP_LOGI(kTag, "Switching effect to: %s", path.c_str());
		auto effect = get_effect(path);
		if (effect) {
			current_effect_ = effect;
			ESP_LOGI(kTag, "Effect switched to: %s", current_effect_->get_name().c_str());
		} else {
			ESP_LOGE(kTag, "get_effect returned nullptr for path: %s", path.c_str());
		}
	}
}

void LightEffectManager::update(const std::vector<std::string> &keys) {
	for (const std::string &key : keys) {
		update(key);
	}
}

void LightEffectManager::set_speed(float new_speed) { speed_ = new_speed; }

float LightEffectManager::get_speed() const { return speed_; }

esp_err_t LightEffectManager::start() {
	if (TaskHandle::x_light_effect_manager_task_handle != nullptr) {
		return ESP_ERR_INVALID_STATE;
	}

	xTaskCreatePinnedToCore(effect_task, "effect_manager", TaskHandle::kStackSizeLightEffectManager, this, TaskHandle::kPrioLightEffectManager,
							&TaskHandle::x_light_effect_manager_task_handle, TaskHandle::kStackCoreLightEffectManager);

	return ESP_OK;
}

void LightEffectManager::effect_task(void *arg) {
	auto *manager = static_cast<LightEffectManager *>(arg);

	while (true) {
		float current_speed = manager->get_speed();
		current_speed = std::max(current_speed, 0.01F);
		// ca. 30 FPS bei speed = 1.0f aktuell
		auto fps = 30.0F;
		auto delay_ms = static_cast<uint32_t>((1000.0F / fps) / current_speed);

		manager->run();
		if (log_counter % 500 == 0) {
			ESP_LOGD(kTag, "Effect task running at speed %.2f with delay %d ms", current_speed, delay_ms);
		}
		log_counter++;
		vTaskDelay(pdMS_TO_TICKS(delay_ms));
	}
}