#pragma once
#include <cstddef>
#include <driver/rmt_tx.h>

/**
 *
 *
 * Based on the: https://github.com/espressif/esp-idf/blob/v6.0.1/examples/peripherals/rmt/led_strip_simple_encoder/main/led_strip_example_main.c
 */
size_t led_strip_encoder(const void *data, size_t data_size, size_t symbols_written, size_t symbols_free, rmt_symbol_word_t *symbols, bool *done,
						 void *arg);