#pragma once

#include <cstdint>
/// @brief Ein RGB-Wert für eine LED
struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

/// @brief Ein Frame, der die LED-Daten für alle LEDs enthält
// Jeder Effect hat einen LED Frame
// Ein LED Frame wird später statisch sein, er wird vom Licht-Effekt Manager
// beschrieben und vom Output Layer gelesen
struct LedFrame {

  static constexpr uint16_t WIDTH = 54;
  static constexpr uint16_t HEIGHT = 60;

  RGB led_data[WIDTH][HEIGHT];
};