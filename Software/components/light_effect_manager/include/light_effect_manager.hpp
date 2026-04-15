#pragma once

#include "config_observer.hpp"
#include "effect.hpp"
#include "led_frame.hpp"

#include <esp_err.h>
#include <string>
#include <vector>

// Diese Klassen werden später von anderen Teammitgliedern
// implementiert, hier nur als Platzhalter referenziert.
class ConfigManager;
class TimeApi;

// Verwaltet den aktuellen Effekt, erzeugt LED-Daten und gibt
// diese über eine Schnittstelle an den späteren Output Layer.
class LightEffectManager : public ConfigObserver {
  public:
	LightEffectManager(LedFrame &external_frame, ConfigManager *config_manager = nullptr, TimeApi *time_api = nullptr);
	~LightEffectManager() = default;

	// Effektverwaltung
	esp_err_t set_effect(Effect *effect);

	// Funktioniert nur, wenn der Effekt vorher registriert wurde.
	esp_err_t set_effect(const char *path);

	esp_err_t register_effect(Effect *effect);
	esp_err_t unregister_effect(Effect *effect);

	Effect *get_effect() const;

	// Update-Logik
	esp_err_t run(const DateTime &time_stamp);

	esp_err_t run();

	esp_err_t start();

	// System-Anbindung & Konfiguration
	void set_config_manager(ConfigManager *config_manager);
	void set_time_api(TimeApi *time_api);

	// Observer Interface
	void update(const std::string &key) override;
	void update(const std::vector<std::string> &keys) override;

	// Attribut aus dem Klassendiagramm
	void set_speed(float new_speed);
	float get_speed() const;

  private:
	static constexpr const char *kTag = "light-effect-manager";
	LedFrame &data;
	ConfigManager *config_manager = nullptr;
	Effect *current_effect = nullptr;
	float speed = 1.0f;
	TimeApi *time_api = nullptr; // später nutzen

	std::vector<Effect *> available_effects;

	static void effect_task(void *arg);
};