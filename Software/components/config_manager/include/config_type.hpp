#pragma once
#include <filesystem>
#include <string>


struct ConfigType {
	std::filesystem::path current_effect;
	std::filesystem::path effects_path;

	ConfigType() = default;
};