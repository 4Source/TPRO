#pragma once
#include "effect.hpp"
#include "effect/blinking_effect.hpp"
#include <map>

// Jeder Effekt im vektor _subeffects hat eine cycle time
struct TimelineStep {
	std::shared_ptr<Effect> effect = nullptr;
	uint32_t duration_ms = 0;
	uint32_t start_ms = 0;
	uint32_t id = 0;
};

struct TimelineSecondaryItem {
	uint32_t id = 0;
	uint32_t cycle_time = 0;
	uint32_t interrupt_time = 0; // Dauer des Interrupts in ms

	struct {
		uint8_t month = 0; // 0 = jeden Monat
		uint8_t day = 0;   // 0 = jeden Tag
		int8_t hour = -1;  // -1 = jede Stunde
		uint8_t minute = 0;
	} trigger;

	std::map<std::string, std::string> overwrites;

	bool is_active(const DateTime::TimeComponents &now) const {
		if (trigger.month != 0 && trigger.month != now.month)
			return false;
		if (trigger.day != 0 && trigger.day != now.day)
			return false;

		if (trigger.hour == -1 || trigger.hour == (int8_t)now.hour) {
			uint32_t trigger_ms, current_ms;
			if (trigger.hour == -1) {
				trigger_ms = trigger.minute * 60000;
				current_ms = (now.minute * 60000) + (now.second * 1000) + now.millisecond;
			} else {
				trigger_ms = (trigger.hour * 3600000) + (trigger.minute * 60000);
				current_ms = (now.hour * 3600000) + (now.minute * 60000) + (now.second * 1000) + now.millisecond;
			}
			return (current_ms >= trigger_ms && current_ms < (trigger_ms + interrupt_time));
		}
		return false;
	}

	uint32_t get_elapsed_ms(const DateTime::TimeComponents &now) const {
		uint32_t current_ms, trigger_ms;
		if (trigger.hour == -1) {
			trigger_ms = trigger.minute * 60000;
			current_ms = (now.minute * 60000) + (now.second * 1000) + now.millisecond;
		} else {
			trigger_ms = (trigger.hour * 3600000) + (trigger.minute * 60000);
			current_ms = (now.hour * 3600000) + (now.minute * 60000) + (now.second * 1000) + now.millisecond;
		}
		return current_ms - trigger_ms;
	}
};

struct TimelineSubeffectConfig {
	std::string type;
	std::string path;
};

/// @brief Spezieller Effect: Timeline aus n verschiedenen Sub-Effekten
/// Muss Effect implementieren: sonst nicht vom Licht-Effekt Manager erkannt
class TimelineEffect : public Effect {
  public:
	TimelineEffect() = default;
	~TimelineEffect() override = default;

	TimelineEffect(const TimelineEffect &) = delete;
	TimelineEffect &operator=(const TimelineEffect &) = delete;
	TimelineEffect(TimelineEffect &&) = delete;
	TimelineEffect &operator=(TimelineEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	// Pure Getter - keine internen veränderungen!
	[[nodiscard]] const std::vector<std::shared_ptr<Effect>> &get_subeffects() const;

	static constexpr const char *kType = "timeline";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/timeline.json";

  private:
	std::vector<std::shared_ptr<Effect>> subeffects_;
	std::vector<std::pair<std::string, std::string>> overwrites_; // Key Value pairs für überschreiben von Sub-Effekt Parametern
	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;
	std::vector<TimelineStep> steps_;
	uint32_t primary_id_{0};
	std::string version_{"1.0"};
	std::string name_{"Timeline Effect"};

	std::vector<TimelineSubeffectConfig> subeffect_configs_;
	std::vector<TimelineSecondaryItem> secondary_items_;

	uint32_t total_duration_ms_{0};
	uint32_t primary_cycle_time_{0};
	uint8_t speed_{1};

	esp_err_t delete_subeffect(Effect *effect);
	esp_err_t set_subeffect(const std::shared_ptr<Effect> &effect);
	esp_err_t initialize_subeffects(); // init subeffects aus subeffect_configs

	// json helper
	esp_err_t parse_static_fields(jparse_ctx_t *jctx);
	esp_err_t parse_dynamic_secondary(const char *raw_json);

	std::string current_active_name_{""}; // Logging
	esp_err_t build_timeline_steps();
};
