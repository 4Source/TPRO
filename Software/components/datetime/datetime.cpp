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

DateTime::TimeComponents DateTime::add_days(const DateTime::TimeComponents &base, int days) {
	struct tm tm_stamp = {};
	// tm_year: Jahre seit 1900
	tm_stamp.tm_year = (base.year > 1900) ? (base.year - 1900) : 0;
	// tm_mon: 0-basiert (Januar = 0)
	tm_stamp.tm_mon = (base.month > 0) ? (base.month - 1) : 0;
	tm_stamp.tm_mday = base.day;
	tm_stamp.tm_hour = base.hour;
	tm_stamp.tm_min = base.minute;
	tm_stamp.tm_sec = base.second;
	tm_stamp.tm_isdst = -1;

	time_t epoch = mktime(&tm_stamp);

	if (epoch == (time_t)-1) {
		return base;
	}

	epoch += (static_cast<time_t>(days) * 86400);

	struct tm result_tm;
	localtime_r(&epoch, &result_tm);

	DateTime::TimeComponents result = {};
	result.year = static_cast<uint16_t>(result_tm.tm_year + 1900);
	result.month = static_cast<uint8_t>(result_tm.tm_mon + 1);
	result.day = static_cast<uint8_t>(result_tm.tm_mday);
	result.hour = static_cast<uint8_t>(result_tm.tm_hour);
	result.minute = static_cast<uint8_t>(result_tm.tm_min);
	result.second = static_cast<uint8_t>(result_tm.tm_sec);
	result.millisecond = base.millisecond; // Bleibt bei Tages-Addition gleich

	return result;
}