#include "timeserver.hpp"
#include <array>
#include <cstring>
#include <ctime>
#include <esp_log.h>
#include <esp_sntp.h>
#include <span>
#include <sys/time.h>

static constexpr const char *kTag = "timeserver";

// timeservers, first choice is Physikalisch-Technische Bundesanstalt in Braunschweig
static const std::array<const char *, 2> kGListOfTimeservers = {{"ptbtime1.ptb.de", "pool.ntp.org"}};

// is time synchronized?
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static bool g_time_synchronized = false;

static int currently_used_server = 0;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void time_received(struct timeval *timeval) {
	// time has been synchronized
	g_time_synchronized = true;

	ESP_LOGI(kTag, "Time synchronized successfully %s", get_current_time_str().c_str());
}
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void init_timeserver() {
	auto servers = std::span{kGListOfTimeservers};
	ESP_LOGI(kTag, "Initializing timeserver");

	// List of timeservers
	ESP_LOGI(kTag, "List of timeservers: %s, %s", servers[0], servers[1]);

	// Set a timeserver
	esp_sntp_setservername(currently_used_server, servers[currently_used_server]);
	ESP_LOGI(kTag, "using Timeserver: %s", servers[currently_used_server]);

	// timeserver should be polled every POLLING_INTERVAL milliseconds
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_set_sync_interval(CONFIG_POLLING_INTERVAL);

	esp_sntp_set_time_sync_notification_cb(time_received);

	// start
	esp_sntp_init();

	// Set timezone to see the timezones see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
	setenv("TZ", CONFIG_TIMEZONE, 1);
	tzset();
	ESP_LOGI(kTag, "Set timezone: %s", CONFIG_TIMEZONE);
}

std::string get_current_time_str() {
	time_t now = 0;
	std::string time_string(64, '\0');
	struct tm timeinfo;

	time(&now);

	// Convert the current time to formatted string
	localtime_r(&now, &timeinfo);
	size_t len = strftime(time_string.data(), time_string.size(), "%d.%m.%Y %H:%M:%S", &timeinfo);

	// trim to actual length
	if (len > 0) {
		time_string.resize(len);
	} else {
		ESP_LOGE(kTag, "Failed to format current time");
		time_string.clear();
	}

	return time_string;
}

bool is_time_synchronized() {
	if (!g_time_synchronized) {
		ESP_LOGI(kTag, "Time is not synchronized yet");
	}
	return g_time_synchronized;
}

void try_next_timeserver() {
	esp_sntp_stop();

	auto servers = std::span{kGListOfTimeservers};
	// next server
	currently_used_server++;

	// no longer synchronized
	g_time_synchronized = false;

	// out of bounds, use first timeserver again
	if (currently_used_server >= servers.size()) {
		currently_used_server = 0;
	}
	ESP_LOGI(kTag, "switched to timeserver %s", servers[currently_used_server]);

	// restart with new timeserver
	init_timeserver();
}

void stop_timeserver() {
	ESP_LOGI(kTag, "Stopped timeserver");
	// no longer synchronized
	g_time_synchronized = false;
	esp_sntp_stop();
}