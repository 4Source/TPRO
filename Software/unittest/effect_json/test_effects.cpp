#include "effect/blinking_effect.hpp"
#include "effect/day_night_effect.hpp"
#include "effect/timeline_effect.hpp"
#include "effect_factory.hpp"
#include "file_manager.hpp"
#include <gtest/gtest.h>

// --- JSON RAW DATA ---

const std::string JSON_DAY_NIGHT = R"({
  "version": "1.0",
  "name": "Day Night",
  "effect": {
      "type": "day_night",
      "parameters": { "default_brightness": 100, "default_speed": 1000 }
  }
})";

const std::string JSON_BLINK = R"({
  "version": "1.0",
  "name": "Blink",
  "effect": {
      "type": "blink",
      "parameters": {
        "color": "#222222",
        "cycle_time": 5000,
        "led_range": [0, 3024],
        "default_brightness": 90
      }
  }
})";

const std::string JSON_TIMELINE = R"({
  "version": "1.0",
  "name": "Main Timeline",
  "effect": {
      "type": "timeline",
      "parameters": {
        "primary": { "id": 0, "cycle_time": 500 },
        "secondary": [ { "id": 1, "cycle_time": 500, "overwrite": { "color": "#FF0000" } } ]
      }
  },
  "subeffects": [
    { "type": "day_night", "path": "/effects/day_night.json" },
    { "type": "blink", "path": "/effects/blink.json" }
  ]
})";

// --- TESTS ---

class EffectIntegrationTest : public ::testing::Test {
  protected:
	void SetUp() override {
		FileManager::clear();
		// "Dateisystem" befüllen
		FileManager::add_mock_file("/effects/day_night.json", JSON_DAY_NIGHT);
		FileManager::add_mock_file("/effects/blink.json", JSON_BLINK);
		FileManager::add_mock_file("/effects/main.json", JSON_TIMELINE);
	}
};

// Testet das Laden der Timeline inklusive der Sub-Effekte
TEST_F(EffectIntegrationTest, TestFullTimelineDeserialization) {
	auto effect = EffectFactory::generate_from_json("/effects/main.json");

	ASSERT_NE(effect, nullptr);
	auto timeline = dynamic_cast<TimelineEffect *>(effect.get());
	ASSERT_NE(timeline, nullptr);

	// EXPECT_EQ(timeline->get_subeffects().size(), 2); TODO: Subeffets sollten auch noch getestet werden!
}

// Testet ob Serialize wieder ein gültiges JSON erzeugt
TEST_F(EffectIntegrationTest, TestBlinkingSerialization) {
	auto effect = EffectFactory::generate_from_json("/effects/blink.json");
	ASSERT_NE(effect, nullptr);

	// Serialisieren (schreibt in den FileManager Mock)
	esp_err_t err = effect->serialize("/effects/output_blink.json");
	EXPECT_EQ(err, 0); // ESP_OK

	// Prüfen ob Datei existiert und Inhalt hat
	auto saved_json = FileManager::read_file("/effects/output_blink.json");
	ASSERT_TRUE(saved_json.has_value());
	EXPECT_TRUE(saved_json->find("\"type\": \"blink\"") != std::string::npos);
}