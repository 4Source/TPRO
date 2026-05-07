#pragma once

#include "datetime.hpp"
#include "effect.hpp"
#include <array>
#include <string>

#pragma pack(push, 1)
struct ContinentNode {
	uint8_t x, y;
	uint8_t North, South, Europe, Africa, Asia, Australia;
};
#pragma pack(pop)

class ContinentEffect : public Effect {
  public:
	ContinentEffect();
	~ContinentEffect() override = default;

	ContinentEffect(const ContinentEffect &) = delete;
	ContinentEffect &operator=(const ContinentEffect &) = delete;
	ContinentEffect(ContinentEffect &&) = delete;
	ContinentEffect &operator=(ContinentEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;
	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;
	esp_err_t set_parameter(const char *name, const char *value) override;
	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;
	std::string get_name() override;

	static constexpr const char *kType = "continent";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/continent.json";

  private:
	std::string path_{kDefaultConfigPath};
	std::string name_{"Continent Effect"};
	std::string version_{"1.0"};
	Param<uint8_t> default_brightness_{100, {"number", 0, 255, 1}};

	Param<RGB> color_north_{RGB{.red = static_cast<uint8_t>(68), .green = static_cast<uint8_t>(214), .blue = static_cast<uint8_t>(44)}, {"color"}};
	Param<RGB> color_south_{RGB{.red = static_cast<uint8_t>(215), .green = static_cast<uint8_t>(5), .blue = static_cast<uint8_t>(132)}, {"color"}};
	Param<RGB> color_europe_{RGB{.red = static_cast<uint8_t>(2), .green = static_cast<uint8_t>(124), .blue = static_cast<uint8_t>(201)}, {"color"}};
	Param<RGB> color_africa_{RGB{.red = static_cast<uint8_t>(255), .green = static_cast<uint8_t>(80), .blue = static_cast<uint8_t>(0)}, {"color"}};
	Param<RGB> color_asia_{RGB{.red = static_cast<uint8_t>(255), .green = static_cast<uint8_t>(239), .blue = static_cast<uint8_t>(0)}, {"color"}};
	Param<RGB> color_australia_{RGB{.red = static_cast<uint8_t>(254), .green = static_cast<uint8_t>(3), .blue = static_cast<uint8_t>(62)}, {"color"}};
	Param<RGB> color_ocean_{RGB{.red = static_cast<uint8_t>(0), .green = static_cast<uint8_t>(0), .blue = static_cast<uint8_t>(0)}, {"color"}};

	// Internal
	static constexpr int ROWS = CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS + CONFIG_LED_CH3_ROWS;
	static constexpr int COLS = CONFIG_LED_COLUMNS;

	std::shared_ptr<std::vector<ContinentNode>> _land_pixels;

	esp_err_t setWeight(int pos_x, int pos_y, const ContinentNode &weight);
};