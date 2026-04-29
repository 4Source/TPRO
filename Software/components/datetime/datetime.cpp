#include "datetime.hpp"
#include <charconv>
#include <format>

DateTime::DateTime() : DateTime{time(nullptr)} {}

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
	: year{year}, month{month}, day{day}, hour{hour}, minute{minute}, second{second} {};

DateTime::DateTime(time_t time) {
	struct tm timeinfo;

	localtime_r(&time, &timeinfo);

	year = static_cast<uint16_t>(timeinfo.tm_year + 1900);
	month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
	day = static_cast<uint8_t>(timeinfo.tm_mday);
	hour = static_cast<uint8_t>(timeinfo.tm_hour);
	minute = static_cast<uint8_t>(timeinfo.tm_min);
	second = static_cast<uint8_t>(timeinfo.tm_sec);
}

DateTime::DateTime(const std::string &time_string) {
	const char *p_time_string = time_string.data();
	int offset = 0;

	auto parse = [&](int length) {
		unsigned int value = 0;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		std::from_chars(p_time_string + offset, p_time_string + offset + length, value);
		offset += length;
		return value;
	};

	// Converts a string from this format '{:02}.{:02}.{:04} {:02}:{:02}:{:02}'
	day = parse(2);
	offset++; // Ignore '.'
	month = parse(2);
	offset++; // Ignore '.'
	year = parse(4);
	offset++; // Ignore ' '
	hour = parse(2);
	offset++; // Ignore ':'
	minute = parse(2);
	offset++; // Ignore ':'
	second = parse(2);
}

std::string DateTime::to_string() const { return std::format("{:02}.{:02}.{:04} {:02}:{:02}:{:02}", day, month, year, hour, minute, second); }
