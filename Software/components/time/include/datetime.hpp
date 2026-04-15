#pragma once
#include <cstdint>
#include <string>
#include <sys/time.h>

// Timestamp
struct DateTime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

	/**
	 * Creates a DateTime object with the current time
	 */
	DateTime();

	/**
	 * Creates a DateTime object with the given values
	 */
	DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

	/**
	 * Creates a DateTime object with the given time
	 */
	DateTime(time_t time);

	/**
	 * Creates a DateTime object from a sting which is formatted as 'dd.mm.yyyy hh:mm:ss'
	 */
	DateTime(const std::string &time_string);

	/**
	 * Returns the DateTime formatted as 'dd.mm.yyyy hh:mm:ss'
	 */
	std::string to_string() const;
};