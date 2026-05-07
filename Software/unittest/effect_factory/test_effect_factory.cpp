#include "effect/blinking_effect.hpp"
#include "effect/day_night_effect.hpp"
#include "effect/timeline_effect.hpp"
#include "effect_factory.hpp"
#include "file_manager.hpp" // Dein aktualisierter Mock Header mit mock_fs
#include <gtest/gtest.h>

class EffectFactoryTest : public ::testing::Test {
  protected:
	void SetUp() override {
		// "Dateisystem" vor jedem Test leeren
		FileManager::mock_fs.clear();
	}

	// Hilfsmethode um Schreibarbeit zu sparen
	void mock_file(const std::string &path, const std::string &content) { FileManager::mock_fs[path] = content; }
};

// --- Positive Test Cases ---

TEST_F(EffectFactoryTest, CreatesDayNightEffect) {
	mock_file("day_night.json", R"({"effect": {"type": "day_night"}})");

	auto effect = EffectFactory::generate_from_json("day_night.json");

	ASSERT_NE(effect, nullptr);
	EXPECT_NE(dynamic_cast<DayNightEffect *>(effect.get()), nullptr);
}

TEST_F(EffectFactoryTest, CreatesBlinkingEffect) {
	mock_file("blink.json", R"({"effect": {"type": "blink"}})");

	auto effect = EffectFactory::generate_from_json("blink.json");

	ASSERT_NE(effect, nullptr);
	EXPECT_NE(dynamic_cast<BlinkingEffect *>(effect.get()), nullptr);
}

TEST_F(EffectFactoryTest, CreatesTimelineEffect) {
	// Da Timeline intern Dateien nachlädt, müssen wir hier ggf. auch Sub-Files mocken
	mock_file("timeline.json", R"({"effect": {"type": "timeline"}})");

	auto effect = EffectFactory::generate_from_json("timeline.json");

	ASSERT_NE(effect, nullptr);
	EXPECT_NE(dynamic_cast<TimelineEffect *>(effect.get()), nullptr);
}

// --- Error Cases ---

TEST_F(EffectFactoryTest, ReturnsNullOnFileNotFound) {
	// mock_fs ist leer, Datei existiert also nicht
	auto effect = EffectFactory::generate_from_json("non_existent.json");

	EXPECT_EQ(effect, nullptr);
}

TEST_F(EffectFactoryTest, ReturnsNullOnUnknownType) {
	mock_file("unknown.json", R"({"effect": {"type": "party_mode"}})");

	auto effect = EffectFactory::generate_from_json("unknown.json");

	EXPECT_EQ(effect, nullptr);
}

TEST_F(EffectFactoryTest, ReturnsNullOnInvalidJson) {
	// Kaputtes JSON-Format
	mock_file("broken.json", R"({"effect": {"type": )");

	auto effect = EffectFactory::generate_from_json("broken.json");

	EXPECT_EQ(effect, nullptr);
}

TEST_F(EffectFactoryTest, ReturnsNullOnMissingEffectKey) {
	mock_file("missing.json", R"({"wrong_key": "nothing"})");

	auto effect = EffectFactory::generate_from_json("missing.json");

	EXPECT_EQ(effect, nullptr);
}

// --- Integration Test: Timeline mit Sub-Effekten ---

TEST_F(EffectFactoryTest, TimelineLoadsSubEffectsFromMockFS) {
	// 1. Die Sub-Effekte bereitstellen
	mock_file("/eff/red.json", R"({"effect": {"type": "blink", "parameters": {"color": "#FF0000"}}})");

	// 2. Die Timeline bereitstellen, die auf das Sub-File verweist
	mock_file("/eff/main.json", R"({
        "effect": {"type": "timeline"},
        "subeffects": [
            {"type": "blink", "path": "/eff/red.json"}
        ]
    })");

	auto effect = EffectFactory::generate_from_json("/eff/main.json");

	ASSERT_NE(effect, nullptr);
	// Hier würde die Timeline jetzt intern FileManager::read_file("/eff/red.json")
	// aufrufen und dank mock_fs den blink-Inhalt finden!
}