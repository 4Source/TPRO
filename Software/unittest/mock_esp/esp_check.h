#pragma once

#include "esp_err.h"
#include <stdio.h>

/**
 * @brief Mock für ESP_RETURN_ON_ERROR
 * Prüft den Rückgabewert. Wenn er nicht ESP_OK ist, gibt er den Fehler zurück.
 */
#define ESP_RETURN_ON_ERROR(x, log_tag, format, ...)                                                                                                 \
	do {                                                                                                                                             \
		esp_err_t err_rc_ = (x);                                                                                                                     \
		if (err_rc_ != ESP_OK) {                                                                                                                     \
			printf("[%s] Error: " format "\n", log_tag, ##__VA_ARGS__);                                                                              \
			return err_rc_;                                                                                                                          \
		}                                                                                                                                            \
	} while (0)

/**
 * @brief Mock für ESP_EXIT_ON_ERROR
 * Ähnlich wie Return, springt aber zu einem Label (oft 'cleanup')
 */
#define ESP_GOTO_ON_ERROR(x, goto_tag, log_tag, format, ...)                                                                                         \
	do {                                                                                                                                             \
		esp_err_t err_rc_ = (x);                                                                                                                     \
		if (err_rc_ != ESP_OK) {                                                                                                                     \
			printf("[%s] Goto %s due to error: " format "\n", log_tag, #goto_tag, ##__VA_ARGS__);                                                    \
			goto goto_tag;                                                                                                                           \
		}                                                                                                                                            \
	} while (0)

/**
 * @brief Mock für ESP_RETURN_ON_FALSE
 * Prüft eine Bedingung (keinen esp_err_t)
 */
#define ESP_RETURN_ON_FALSE(a, err_code, log_tag, format, ...)                                                                                       \
	do {                                                                                                                                             \
		if (!(a)) {                                                                                                                                  \
			printf("[%s] False condition: " format "\n", log_tag, ##__VA_ARGS__);                                                                    \
			return err_code;                                                                                                                         \
		}                                                                                                                                            \
	} while (0)
