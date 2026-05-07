#pragma once
#include "led_frame.hpp" // Nimmt an, dass RGB dort definiert ist

// Konvertiert HSV (Hue: 0-360, Saturation: 0-1, Value: 0-1) zu RGB
[[nodiscard]] RGB hsv_to_rgb(float h, float s, float v);