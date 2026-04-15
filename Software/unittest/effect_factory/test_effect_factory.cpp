#include "blinking_effect.hpp"
#include "day_night_effect.hpp"
#include "effect_factory.hpp"
#include "file_manager.hpp" // Mock Heqader
#include "timeline_effect.hpp"
#include <gtest/gtest.h>

class EffectFactoryTest : public ::testing::Test {
  protected:
	void SetUp() override {
		// Reset des Mocks vor jedem Test
		FileManager::mock_content = nullptr;
	}
};

// day_night
TEST_F(EffectFactoryTest, CreatesDayNightEffect) {
	FileManager::mock_content = R"({"effect": {"type": "day_night"}})";

	auto effect = EffectFactory::generate_from_json("dummy.json");

	ASSERT_NE(effect, nullptr);
	// Prüfen, ob der Typ korrekt ist (RTTI muss aktiviert sein)
	EXPECT_NE(dynamic_cast<DayNightEffect *>(effect.get()), nullptr);
}

// blinking
TEST_F(EffectFactoryTest, CreatesBlinkingEffect) {
	FileManager::mock_content = R"({"effect": {"type": "blinking"}})";

	auto effect = EffectFactory::generate_from_json("dummy.json");

	ASSERT_NE(effect, nullptr);
	EXPECT_NE(dynamic_cast<BlinkingEffect *>(effect.get()), nullptr);
}

// timeline
TEST_F(EffectFactoryTest, CreatesTimelineEffect) {
	FileManager::mock_content = R"({"effect": {"type": "timeline"}})";

	auto effect = EffectFactory::generate_from_json("dummy.json");

	ASSERT_NE(effect, nullptr);
	EXPECT_NE(dynamic_cast<TimelineEffect *>(effect.get()), nullptr);
}

// Error Cases
TEST_F(EffectFactoryTest, ReturnsNullOnUnknownType) {
	FileManager::mock_content = R"({"effect": {"type": "party_mode"}})";

	auto effect = EffectFactory::generate_from_json("unknown.json");

	EXPECT_EQ(effect, nullptr);
}

// Ungültige json
TEST_F(EffectFactoryTest, ReturnsNullOnInvalidJson) {
	FileManager::mock_content = R"({"effect": {"type": )";

	auto effect = EffectFactory::generate_from_json("broken.json");

	EXPECT_EQ(effect, nullptr);
}

// Kein "effect"
TEST_F(EffectFactoryTest, ReturnsNullOnMissingEffectKey) {
	FileManager::mock_content = R"({"wrong_key": "nothing"})";

	auto effect = EffectFactory::generate_from_json("missing.json");

	EXPECT_EQ(effect, nullptr);
}