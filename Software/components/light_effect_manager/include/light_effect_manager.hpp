#pragma once

#include "config_observer.hpp"
#include "effect.hpp"
#include "effect_factory.hpp"
#include "led_frame.hpp"

#include <esp_err.h>
#include <esp_log.h>
#include <memory>
#include <string>
#include <vector>

// Diese Klassen werden später von anderen Teammitgliedern
// implementiert, hier nur als Platzhalter referenziert.
class ConfigManager;

// Verwaltet den aktuellen Effekt, erzeugt LED-Daten und gibt
// diese über eine Schnittstelle an den späteren Output Layer.
class LightEffectManager : public ConfigObserver {
  public:
	LightEffectManager(LedFrame &external_frame, ConfigManager *config_manager = nullptr);
	~LightEffectManager() override = default;

	LightEffectManager(const LightEffectManager &) = delete;
	LightEffectManager &operator=(const LightEffectManager &) = delete;
	LightEffectManager(LightEffectManager &&) = delete;
	LightEffectManager &operator=(LightEffectManager &&) = delete;

	// Effektverwaltung
	esp_err_t set_effect(const std::shared_ptr<Effect> &effect);

	// Funktioniert nur, wenn der Effekt vorher registriert wurde.
	esp_err_t set_effect(const std::string &path);

	esp_err_t register_effect(const std::shared_ptr<Effect> &effect);
	esp_err_t unregister_effect(const std::shared_ptr<Effect> &effect);

	// get effect via path when effects doesnt exist create it via factory
	std::shared_ptr<Effect> get_effect(const std::string &path);

	// Update-Logik
	esp_err_t run();

	esp_err_t start();

	// System-Anbindung & Konfiguration
	void set_config_manager(ConfigManager *config_manager);

	// Observer Interface
	void update(const std::string &key) override;
	void update(const std::vector<std::string> &keys) override;

	// Attribut aus dem Klassendiagramm
	void set_speed(float new_speed);
	[[nodiscard]] float get_speed() const;

  private:
	static constexpr const char *kTag = "light-effect-manager";
	LedFrame &data_;
	LedFrame working_frame_;
	ConfigManager *config_manager_ = nullptr;
	std::shared_ptr<Effect> current_effect_ = nullptr;
	float speed_ = 1.0F;

	std::vector<std::shared_ptr<Effect>> available_effects_;

	static void effect_task(void *arg);
};