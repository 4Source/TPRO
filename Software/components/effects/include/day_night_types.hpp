#pragma once
#include "file_manager.hpp"
#include <cstdint>
#include <esp_err.h>
#include <memory>
#include <vector>

namespace DayNight {

#pragma pack(push, 1)
struct DayNightData {
	uint8_t x_pos;
	uint8_t y_pos;
	int16_t sunrise_min;
	int16_t sunset_min;
	float lat;
	float lon;
};
#pragma pack(pop)

/**
 * @brief Globaler Shared Pointer für die Mapping-Daten.
 * Durch 'inline' (C++17) wird er in allen Übersetzungseinheiten identisch definiert.
 */
inline std::shared_ptr<std::vector<DayNightData>> shared_led_data = nullptr;

esp_err_t load_leds_streaming(const std::string &path);
esp_err_t load_day_from_file(const std::string &path);

} // namespace DayNight