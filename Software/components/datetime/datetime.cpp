#include "datetime.hpp"
#include <ctime>
#include <format>

DateTime::TimeComponents DateTime::get_now() {
	struct timeval time_value;
	gettimeofday(&time_value, nullptr);

	struct tm timeinfo;
	localtime_r(&time_value.tv_sec, &timeinfo);

	return TimeComponents{.year = static_cast<uint16_t>(timeinfo.tm_year + 1900),
						  .month = static_cast<uint8_t>(timeinfo.tm_mon + 1),
						  .day = static_cast<uint8_t>(timeinfo.tm_mday),
						  .hour = static_cast<uint8_t>(timeinfo.tm_hour),
						  .minute = static_cast<uint8_t>(timeinfo.tm_min),
						  .second = static_cast<uint8_t>(timeinfo.tm_sec),
						  .millisecond = static_cast<uint16_t>(time_value.tv_usec / 1000)};
}

std::string DateTime::to_string() { return format(get_now()); }

std::string DateTime::format(const TimeComponents &comps) {
	return std::format("{:02}.{:02}.{:04} {:02}:{:02}:{:02}.{:03}", comps.day, comps.month, comps.year, comps.hour, comps.minute, comps.second,
					   comps.millisecond);
}