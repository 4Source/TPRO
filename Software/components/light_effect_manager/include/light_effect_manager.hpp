#pragma once

#include "effect.hpp"
#include "led_frame.hpp"

#include <esp_err.h>
#include <vector>

// Diese Klassen werden später von anderen Teammitgliedern 
// implementiert, hier nur als Platzhalter referenziert.
class ConfigManager;
class TimeApi;

// Schnittstelle für den späteren Output Layer.
// Ein späterer LED-Controller / Renderer / Hardware-Treiber
// kann diese Schnittstelle implementieren.
class ILedFrameSink {
public:
  virtual ~ILedFrameSink() = default;
  virtual esp_err_t write(const LedFrame &frame) = 0;
};

// Verwaltet den aktuellen Effekt, erzeugt LED-Daten und gibt
// diese über eine Schnittstelle an den späteren Output Layer.
class LightEffectManager {
public:
  LightEffectManager(ConfigManager *config_manager = nullptr,
                     TimeApi *time_api = nullptr);

  ~LightEffectManager() = default;

  // Output-Layer-Anbindung
  // "subscribe / unsubscribe" ist hier für Empfänger gedacht,
  // die LedFrame-Daten erhalten sollen.
  // ----------------------------------------------------------
  esp_err_t subscribe(ILedFrameSink *sink);
  esp_err_t unsubscribe(ILedFrameSink *sink);

  // Effektverwaltung
  esp_err_t set_effect(Effect *effect);

  // Funktioniert nur, wenn der Effekt vorher registriert wurde.
  esp_err_t set_effect(const char *path);

  esp_err_t register_effect(Effect *effect);
  esp_err_t unregister_effect(Effect *effect);

  Effect *get_effect() const;

  // Update-Logik
  esp_err_t update(const DateTime &time_stamp);

  // Platzhalter für spätere TimeApi-Integration
  esp_err_t update();

  // Schreibt den zuletzt berechneten Frame an alle Sinks
  esp_err_t write_led_data();

  // Zugriff auf den letzten berechneten Frame
  const LedFrame &get_led_data() const;



  // Anschlussstellen für spätere Integration
  void set_config_manager(ConfigManager *config_manager);
  void set_time_api(TimeApi *time_api);

  // Platzhalter für spätere ConfigManager-Anbindung
  // Kann später z. B. vom ConfigManager aufgerufen werden,
  // wenn sich die Konfiguration geändert hat.
  esp_err_t on_config_changed();

  // Attribut aus dem Klassendiagramm
  void set_speed(float new_speed);
  float get_speed() const;

private:
  LedFrame data{};
  ConfigManager *config_manager = nullptr; // später nutzen
  Effect *current_effect = nullptr;
  float speed = 1.0f;
  TimeApi *time_api = nullptr;             // später nutzen

  std::vector<ILedFrameSink *> sinks;
  std::vector<Effect *> available_effects;
};