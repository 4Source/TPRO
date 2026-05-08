#pragma once

#include <stdio.h>

// Log-Level Definitionen (müssen mit ESP-IDF übereinstimmen)
typedef enum {
	ESP_LOG_NONE,	/*!< No log output */
	ESP_LOG_ERROR,	/*!< Critical errors, software module can not recover on its own */
	ESP_LOG_WARN,	/*!< Error conditions from which recovery measures have been taken */
	ESP_LOG_INFO,	/*!< Information messages which describe normal flow of events */
	ESP_LOG_DEBUG,	/*!< Extra information which is not necessary for normal operation (values, etc) */
	ESP_LOG_VERBOSE /*!< Bigger chunks of data or details of particular importance; can be inefficient */
} esp_log_level_t;

// Makros für die verschiedenen Log-Stufen
// Wir nutzen __VA_ARGS__ um die Format-Strings und Variablen zu übernehmen
#define ESP_LOGW(tag, format, ...) printf("[WARN ][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) printf("[DEBUG][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) printf("[VERB ][%s] " format "\n", tag, ##__VA_ARGS__)
#ifndef ESP_LOGD
#define ESP_LOGD(tag, format, ...) printf("[INFO ][%s] " format "\n", tag, ##__VA_ARGS__)
#endif
#ifndef ESP_LOGE
#define ESP_LOGE(tag, format, ...) printf("[ERROR][%s] " format "\n", tag, ##__VA_ARGS__)
#endif
// Mock für esp_log_write (falls direkt aufgerufen)
inline void esp_log_write(esp_log_level_t level, const char *tag, const char *format, ...) {
	// Einfachheitshalber hier ignorieren oder ebenfalls auf printf mappen
}

// Mock für die Einstellung des Log-Levels
inline void esp_log_level_set(const char *tag, esp_log_level_t level) {
	// Im Unit-Test meist nicht relevant
}

// Mock für esp_err_to_name (wird oft in Log-Statements genutzt)
inline const char *esp_err_to_name(int code) { return "MOCK_ESP_ERR"; }