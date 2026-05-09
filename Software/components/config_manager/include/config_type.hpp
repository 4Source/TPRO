#pragma once
#include <filesystem>
#include <string>

struct ConfigType {
	std::filesystem::path current_effect{"/effects/defaults/day_night.json"};
	std::filesystem::path effects_path{"/effects"};
	std::string active_from{"07:00"};
	std::string active_to{"17:00"};
	ConfigType() = default;
};
