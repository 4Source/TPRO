#include "datetime.hpp"
#include <gtest/gtest.h>

TEST(DateTimeTest, DefaultConstructor) {
	time_t now = time(nullptr);
	struct tm expected{};

	localtime_r(&now, &expected);

	DateTime dt;

	EXPECT_EQ(dt.year, expected.tm_year + 1900);
	EXPECT_EQ(dt.month, expected.tm_mon + 1);
	EXPECT_EQ(dt.day, expected.tm_mday);
	EXPECT_EQ(dt.hour, expected.tm_hour);
	EXPECT_EQ(dt.minute, expected.tm_min);

	// seconds may differ slightly due to execution delay
	EXPECT_NEAR(dt.second, expected.tm_sec, 1);
}

TEST(DateTimeTest, FromTimeValid) {
	time_t now = time(nullptr);
	struct tm expected{};

	localtime_r(&now, &expected);

	DateTime dt{now};

	EXPECT_EQ(dt.year, expected.tm_year + 1900);
	EXPECT_EQ(dt.month, expected.tm_mon + 1);
	EXPECT_EQ(dt.day, expected.tm_mday);
	EXPECT_EQ(dt.hour, expected.tm_hour);
	EXPECT_EQ(dt.minute, expected.tm_min);
	EXPECT_EQ(dt.second, expected.tm_sec);
}

TEST(DateTimeTest, FromStringValid) {
	DateTime dt("15.04.2026 12:34:56");

	EXPECT_EQ(dt.day, 15);
	EXPECT_EQ(dt.month, 4);
	EXPECT_EQ(dt.year, 2026);
	EXPECT_EQ(dt.hour, 12);
	EXPECT_EQ(dt.minute, 34);
	EXPECT_EQ(dt.second, 56);
}

TEST(DateTimeTest, ToString) {
	DateTime dt(2026, 4, 15, 12, 34, 56);

	EXPECT_EQ(dt.to_string(), "15.04.2026 12:34:56");
}

TEST(DateTimeTest, InvalidString) {
	DateTime dt("invalid");

	EXPECT_EQ(dt.day, 0);
	EXPECT_EQ(dt.month, 0);
	EXPECT_EQ(dt.year, 0);
	EXPECT_EQ(dt.hour, 0);
	EXPECT_EQ(dt.minute, 0);
	EXPECT_EQ(dt.second, 0);
}