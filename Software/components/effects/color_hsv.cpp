#include "color_hsv.hpp"
#include <cmath>

RGB hsv_to_rgb(const float hue, const float saturation, const float value) {
	const float chroma = value * saturation;
	const float chroma_x = chroma * (1.0F - std::abs(std::fmod(hue / 60.0F, 2.0F) - 1.0F));
	const float match = value - chroma;
	float red = 0.0F;
	float green = 0.0F;
	float blue = 0.0F;

	if (hue >= 0.0F && hue < 60.0F) {
		red = chroma;
		green = chroma_x;
		blue = 0.0F;
	} else if (hue >= 60.0F && hue < 120.0F) {
		red = chroma_x;
		green = chroma;
		blue = 0.0F;
	} else if (hue >= 120.0F && hue < 180.0F) {
		red = 0.0F;
		green = chroma;
		blue = chroma_x;
	} else if (hue >= 180.0F && hue < 240.0F) {
		red = 0.0F;
		green = chroma_x;
		blue = chroma;
	} else if (hue >= 240.0F && hue < 300.0F) {
		red = chroma_x;
		green = 0.0F;
		blue = chroma;
	} else {
		red = chroma;
		green = 0.0F;
		blue = chroma_x;
	}

	return RGB{.red = static_cast<uint8_t>((red + match) * 255.0F),
			   .green = static_cast<uint8_t>((green + match) * 255.0F),
			   .blue = static_cast<uint8_t>((blue + match) * 255.0F)};
}