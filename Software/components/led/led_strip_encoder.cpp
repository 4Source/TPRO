#include "led_strip_encoder.hpp"

#include <cmath>
#include <cstdint>
#include <esp_assert.h>
#include <hal/rmt_types.h>
#include <span>

// This is for clang tidy when not configured the files still get analyzed and than have missing defines
#ifdef __clang__
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#ifndef CONFIG_LED_STRIP_RESOLUTION_HZ
#define CONFIG_LED_STRIP_RESOLUTION_HZ 10000000
#endif
#ifndef CONFIG_LED_STRIP_T0H
#define CONFIG_LED_STRIP_T0H 300.0
#endif
#ifndef CONFIG_LED_STRIP_T0L
#define CONFIG_LED_STRIP_T0L 900.0
#endif
#ifndef CONFIG_LED_STRIP_T1H
#define CONFIG_LED_STRIP_T1H 900.0
#endif
#ifndef CONFIG_LED_STRIP_T1L
#define CONFIG_LED_STRIP_T1L 300.0
#endif
#ifndef CONFIG_LED_STRIP_RES
#define CONFIG_LED_STRIP_RES 280000.0
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)
#endif

inline uint16_t to_ticks(float time_ns) {
	auto ticks = std::lround(time_ns * (CONFIG_LED_STRIP_RESOLUTION_HZ / 1000000000.0F));
	if (ticks < 0) {
		return 0;
	}
	if (ticks > 65535) {
		return 65535;
	}
	return static_cast<uint16_t>(ticks);
}

static const rmt_symbol_word_t kWs2815Zero = {
	.duration0 = to_ticks(CONFIG_LED_STRIP_T0H),
	.level0 = 1,
	.duration1 = to_ticks(CONFIG_LED_STRIP_T0L),
	.level1 = 0,
};

static const rmt_symbol_word_t kWs2815One = {
	.duration0 = to_ticks(CONFIG_LED_STRIP_T1H),
	.level0 = 1,
	.duration1 = to_ticks(CONFIG_LED_STRIP_T1L),
	.level1 = 0,
};

static const rmt_symbol_word_t kWs2815Reset = {
	.duration0 = to_ticks(CONFIG_LED_STRIP_RES / 2),
	.level0 = 0,
	.duration1 = to_ticks(CONFIG_LED_STRIP_RES / 2),
	.level1 = 0,
};

size_t led_strip_encoder(const void *data, size_t data_size, size_t symbols_written, size_t symbols_free, rmt_symbol_word_t *symbols, bool *done,
						 void *arg) {
	// Debug check
	assert(symbols_written % 8 == 0);

	// Return if not enough symbol space available to encode a byte.
	if (symbols_free < 8) {
		return 0;
	}

	// We can calculate where in the data we are from the symbol pos.
	// Alternatively, we could use some counter referenced by the arg
	// parameter to keep track of this.
	size_t data_pos = symbols_written / 8;
	std::span<const uint8_t> data_bytes(static_cast<const uint8_t *>(data), data_size);

	std::span<rmt_symbol_word_t> sym(symbols, symbols_free);

	if (data_pos < data_size) {
		// Encode a byte
		size_t symbol_pos = 0;
		uint8_t byte = data_bytes[data_pos];

		for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
			if ((byte & bitmask) != 0) {
				sym[symbol_pos++] = kWs2815One;
			} else {
				sym[symbol_pos++] = kWs2815Zero;
			}
		}
		// We're done; we should have written 8 symbols.
		return symbol_pos;
	}

	// All bytes already are encoded.
	// Encode the reset, and we're done.
	sym[0] = kWs2815Reset;
	*done = true; // Indicate end of the transaction.
	return 1;	  // we only wrote one symbol
}
