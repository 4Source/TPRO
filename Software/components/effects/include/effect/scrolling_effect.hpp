#pragma once

#include "effect.hpp"
#include "lvgl.h"

// Nutze 3x2 LED pro LVGL-Pixel - 5cm/3,3333cm = 3/2
constexpr int kScaleX = 3;
constexpr int kScaleY = 2;
constexpr int kLogicalHeight = 14; // Pixelhöhe 14 -> 28 LEDs von 35 verfügbaren

class ScrollingEffect : public Effect {
  public:
	ScrollingEffect();
	~ScrollingEffect() override;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;
	esp_err_t set_parameter(const char *name, const char *value) override;
	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "scrolling";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/scrolling.json";

  private:
	// 87x35 physische LED
	const uint32_t physical_width_{87};
	uint32_t last_scroll_ms_{0};
	int32_t scroll_offset_{0};

	std::string name_{"Scrolling Effect"};

	Param<std::string> text_{"WILLKOMMEN AUF DER NEUEN MATRIX!", {"string"}};
	Param<float> scroll_speed_{1.0F, {"number", static_cast<int>(0.1F), static_cast<int>(10.0F), static_cast<int>(0.1F)}};

	std::string path_{kDefaultConfigPath};
	std::array<char, 256> path_buffer_{};

	// LVGL
	lv_display_t *disp_{nullptr};
	lv_obj_t *screen_{nullptr};
	lv_obj_t *label_{nullptr};
	std::vector<uint8_t> draw_buffer_;

	std::unique_ptr<LedFrame> current_frame_;

	// LVGL Callback
	static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
};