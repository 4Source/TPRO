#include "light_effect_manager.hpp"
#include "config_manager.hpp"
#include "datetime.hpp"
#include "task_handles.hpp"
#include <algorithm>
#include <cstring>
#include <esp_err.h>
#include <freertos/task.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static uint16_t log_counter = 0;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

LightEffectManager::LightEffectManager(LedFrame &external_frame, ConfigManager *config_manager, TimeApi *time_api)
	: data(external_frame), config_manager(config_manager), time_api(time_api) {}

// ============================================================
// Effektverwaltung
// ============================================================

esp_err_t LightEffectManager::set_effect(const std::shared_ptr<Effect> &effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	current_effect = effect;
	return ESP_OK;
}

esp_err_t LightEffectManager::set_effect(const std::string &path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	// Voraussetzung:
	// - Effekt wurde mit register_effect(...) registriert
	// - Effekt liefert sinnvollen Pfad über get_filepath()

	for (const std::shared_ptr<Effect> &effect : available_effects) {
		if (effect == nullptr) {
			continue;
		}

		const std::string effect_path = effect->get_filepath();
		if (effect_path.empty()) {
			continue;
		}

		if (effect_path == path) {
			current_effect = effect;
			return ESP_OK;
		}
	}

	return ESP_ERR_NOT_FOUND;
}

esp_err_t LightEffectManager::register_effect(const std::shared_ptr<Effect> &effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	auto iterator = std::ranges::find(available_effects, effect);
	if (iterator != available_effects.end()) {
		ESP_LOGW("light-effect-manager", "Effect already registered: %s", effect->get_filepath().c_str());
		return ESP_OK; // schon vorhanden
	}

	available_effects.push_back(effect);
	return ESP_OK;
}

esp_err_t LightEffectManager::unregister_effect(const std::shared_ptr<Effect> &effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	auto iterator = std::ranges::find(available_effects, effect);
	if (iterator == available_effects.end()) {
		return ESP_ERR_NOT_FOUND;
	}

	if (current_effect == effect) {
		current_effect = nullptr;
	}

	available_effects.erase(iterator);
	return ESP_OK;
}

std::shared_ptr<Effect> LightEffectManager::get_effect(const std::string &path) {
	auto iterator = std::ranges::find_if(available_effects, [&path](const auto &effect) { return effect && effect->get_filepath() == path; });

	if (iterator != available_effects.end()) {
		return *iterator;
	}
	// Gibt es noch nicht, versuche zu erzeugen
	auto new_effect = EffectFactory::generate_from_json(path);
	if (new_effect) {
		available_effects.push_back(new_effect);
		return new_effect;
	}

	return nullptr;
}

// ============================================================
// Update-Logik
// ============================================================

esp_err_t LightEffectManager::run(const DateTime &time_stamp) {
	if (current_effect == nullptr) {
		ESP_LOGW(kTag, "current_effect is NULL!");
		return ESP_ERR_INVALID_STATE;
	}

	std::unique_ptr<LedFrame> new_frame = current_effect->get_led_data(time_stamp);

	if (!new_frame) {
		ESP_LOGW(kTag, "Effect returned null frame for timestamp %u-%02u-%02u %02u:%02u:%02u", time_stamp.year, time_stamp.month, time_stamp.day,
				 time_stamp.hour, time_stamp.minute, time_stamp.second);
		return ESP_ERR_NO_MEM;
	}

	if (log_counter % 500 == 0) {
		ESP_LOGI(kTag, "Effect generated new frame for timestamp %u-%02u-%02u %02u:%02u:%02u", time_stamp.year, time_stamp.month, time_stamp.day,
				 time_stamp.hour, time_stamp.minute, time_stamp.second);
		ESP_LOGI(kTag, "New frame data (first LED): R=%d G=%d B=%d", new_frame->led_data[0][0].red, new_frame->led_data[0][0].green,
				 new_frame->led_data[0][0].blue);
	}

	bool frame_changed = false;
	{
		LedFrame::ScopedWriteLock write_lock(data);

		// LED-Daten vergleichen und kopieren
		if (data.led_data != new_frame->led_data) {
			data.copy_data_from(*new_frame);
			frame_changed = true;
		}
	}

	// Tasks aufwecken nur bei Änderungen
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

esp_err_t LightEffectManager::run() { // NOLINT(readability-convert-member-functions-to-static) because there is no real implementation
	// ==========================================================
	// TODO: Spätere TimeApi-Integration
	//
	// Sobald TimeApi implementiert wurde,
	// kann diese Methode z. B. so erweitert werden:
	//
	// 1. Prüfen, ob time_api gesetzt ist
	// 2. Aktuelle Zeit über time_api anfragen
	// 3. In DateTime umwandeln
	// 4. run(date_time) aufrufen
	//
	//
	//
	// if (time_api == nullptr) {
	//   return ESP_ERR_INVALID_STATE;
	// }
	//
	// auto result = time_api->request();
	// if (!result.valid) {
	//   return ESP_FAIL;
	// }
	//
	// return run(result.date_time);
	// ==========================================================

	return ESP_ERR_NOT_SUPPORTED;
}

// ============================================================
// System-Anbindung & Konfiguration
// ============================================================

void LightEffectManager::set_config_manager(ConfigManager *manager) {
	if (config_manager != nullptr) {
		config_manager->remove_observer("current_effect", *this);
	}

	config_manager = manager;

	if (config_manager != nullptr) {
		config_manager->add_observer("current_effect", *this);
	}
}

void LightEffectManager::set_time_api(TimeApi *api) { time_api = api; }

void LightEffectManager::update(const std::string &key) {
	if (config_manager == nullptr) {
		return;
	}

	if (key == "current_effect") {
		auto path = config_manager->get_config("current_effect");
		set_effect(path);
	}
}

void LightEffectManager::update(const std::vector<std::string> &keys) {
	for (const std::string &key : keys) {
		update(key);
	}
}

void LightEffectManager::set_speed(float new_speed) { speed = new_speed; }

float LightEffectManager::get_speed() const { return speed; }

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
		auto delay_ms = static_cast<uint32_t>(33.0F / current_speed);

		DateTime current_time{};
		manager->run(current_time);
		if (log_counter % 500 == 0) {
			ESP_LOGI(kTag, "Effect task running at speed %.2f with delay %d ms", current_speed, delay_ms);
		}
		log_counter++;
		vTaskDelay(pdMS_TO_TICKS(delay_ms));
	}
}