#include "light_effect_manager.hpp"

#include <algorithm>
#include <cstring>

LightEffectManager::LightEffectManager(ConfigManager *config_manager, TimeApi *time_api) : config_manager(config_manager), time_api(time_api) {}

// ============================================================
// Output-Layer-Anbindung
// ============================================================

esp_err_t LightEffectManager::subscribe(ILedFrameSink *sink) {
	if (sink == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	auto iterator = std::ranges::find(sinks, sink);
	if (iterator != sinks.end()) {
		return ESP_OK; // schon registriert
	}

	sinks.push_back(sink);
	return ESP_OK;
}

esp_err_t LightEffectManager::unsubscribe(ILedFrameSink *sink) {
	if (sink == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	auto iterator = std::ranges::find(sinks, sink);
	if (iterator == sinks.end()) {
		return ESP_ERR_NOT_FOUND;
	}

	sinks.erase(iterator);
	return ESP_OK;
}

// ============================================================
// Effektverwaltung
// ============================================================

esp_err_t LightEffectManager::set_effect(Effect *effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	current_effect = effect;
	return ESP_OK;
}

esp_err_t LightEffectManager::set_effect(const char *path) {
	if (path == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	// Voraussetzung:
	// - Effekt wurde mit register_effect(...) registriert
	// - Effekt liefert sinnvollen Pfad über get_filepath()

	for (Effect *effect : available_effects) {
		if (effect == nullptr) {
			continue;
		}

		const char *effect_path = effect->get_filepath();
		if (effect_path == nullptr) {
			continue;
		}

		if (std::strcmp(effect_path, path) == 0) {
			current_effect = effect;
			return ESP_OK;
		}
	}

	return ESP_ERR_NOT_FOUND;
}

esp_err_t LightEffectManager::register_effect(Effect *effect) {
	if (effect == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	auto iterator = std::ranges::find(available_effects, effect);
	if (iterator != available_effects.end()) {
		return ESP_OK; // schon vorhanden
	}

	available_effects.push_back(effect);
	return ESP_OK;
}

esp_err_t LightEffectManager::unregister_effect(Effect *effect) {
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

Effect *LightEffectManager::get_effect() const { return current_effect; }

// ============================================================
// Update-Logik
// ============================================================

esp_err_t LightEffectManager::update(const DateTime &time_stamp) {
	if (current_effect == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}

	// Der aktuell gesetzte Effekt erzeugt aus der Zeit einen Frame
	data = current_effect->get_led_data(time_stamp);

	// Anschließend wird der Frame an alle registrierten Ausgaben gesendet
	return write_led_data();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
esp_err_t LightEffectManager::update() {
	// ==========================================================
	// TODO: Spätere TimeApi-Integration
	//
	// Sobald TimeApi implementiert wurde,
	// kann diese Methode z. B. so erweitert werden:
	//
	// 1. Prüfen, ob time_api gesetzt ist
	// 2. Aktuelle Zeit über time_api anfragen
	// 3. In DateTime umwandeln
	// 4. update(date_time) aufrufen
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
	// return update(result.date_time);
	// ==========================================================

	return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t LightEffectManager::write_led_data() {
	for (ILedFrameSink *sink : sinks) {
		if (sink == nullptr) {
			continue;
		}

		esp_err_t err = sink->write(data);
		if (err != ESP_OK) {
			return err;
		}
	}

	return ESP_OK;
}

const LedFrame &LightEffectManager::get_led_data() const { return data; }

// ============================================================
// Anschlussstellen für spätere Integration
// ============================================================

void LightEffectManager::set_config_manager(ConfigManager *manager) { config_manager = manager; }

void LightEffectManager::set_time_api(TimeApi *api) { time_api = api; }

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
esp_err_t LightEffectManager::on_config_changed() {
	// ==========================================================
	// TODO: Spätere ConfigManager-Integration
	//
	// Sobald ConfigManager fertig ist, kann diese Methode
	// z. B. folgende Aufgaben übernehmen:
	//
	// - aktuellen Effektpfad aus Konfiguration lesen
	// - speed aus Konfiguration lesen
	// - set_effect(path) aufrufen
	// - ggf. update() oder update(zeit) auslösen
	//
	//
	//
	// if (config_manager == nullptr) {
	//   return ESP_ERR_INVALID_STATE;
	// }
	//
	// auto config = config_manager->get_config(...);
	// set_speed(config.speed);
	// set_effect(config.current_effect_path);
	// ==========================================================

	return ESP_OK;
}

void LightEffectManager::set_speed(float new_speed) { speed = new_speed; }

float LightEffectManager::get_speed() const { return speed; }