#include "blinking_effect.hpp"
#include "light_effect_manager.hpp"
#include <gtest/gtest.h>

// Testet ob Light Effect Manager led_frame (statisch in main) korrekt beschreibt
TEST(LightEffectManagerTest, VerifiesExternalFrameUpdate) {
	// Statische Instanz wie in main.cpp
	LedFrame main_frame;

	// Manager mit Referenz initialisieren
	LightEffectManager manager(main_frame);

	// Effekt registrieren und setzen
	BlinkingEffect blink;
	manager.register_effect(&blink);
	manager.set_effect(&blink);

	// gerade sekunde -> weiß
	DateTime t_on;
	t_on.second = 2;
	manager.run(t_on);

	// Prüfe erstes Pixel
	EXPECT_EQ(main_frame.led_data[0][0].red, 255);
	EXPECT_EQ(main_frame.led_data[0][0].green, 255);
	EXPECT_EQ(main_frame.led_data[0][0].blue, 255);

	// Ungerade sekunde -> schwarz
	DateTime t_off;
	t_off.second = 3;
	manager.run(t_off);

	EXPECT_EQ(main_frame.led_data[0][0].red, 0);
	EXPECT_EQ(main_frame.led_data[0][0].green, 0);
	EXPECT_EQ(main_frame.led_data[0][0].blue, 0);
}