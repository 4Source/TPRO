#pragma once
#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>
#include <sys/time.h>

class DateTime {
  public:
	// Kein Konstruktor nötig, da rein statische Utility-Klasse
	DateTime() = delete;

	// Timestamp
	struct TimeComponents {
		uint16_t year;
		uint8_t month, day, hour, minute, second;
		uint16_t millisecond;
	};

	/**
	 * Gibt die aktuelle Zeit als String im Format 'dd.mm.yyyy hh:mm:ss.ms' zurück
	 */
	[[nodiscard]] static std::string to_string();

	/**
	 * Formatiert spezifische TimeComponents als String
	 */
	[[nodiscard]] static std::string format(const TimeComponents &comps);

	/**
	 * Gibt die aktuelle Zeit als TimeComponents zurück
	 */
	static TimeComponents get_now();
};