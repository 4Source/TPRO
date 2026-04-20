#pragma once
#include <map>
#include <optional>
#include <string>

class FileManager {
  public:
	// Speichert Pfad -> JSON Inhalt
	static inline std::map<std::string, std::string> mock_fs;

	// Hilfsmethode für den Test-Setup
	static void add_mock_file(const std::string &path, const std::string &content) { mock_fs[path] = content; }

	static void clear() { mock_fs.clear(); }

	static std::optional<std::string> read_file(const std::string &path) {
		auto it = mock_fs.find(path);
		if (it != mock_fs.end()) {
			return it->second;
		}
		return std::nullopt;
	}

	static inline int save_file(const std::string &path, const char *data) {
		mock_fs[path] = std::string(data);
		return 0; // ESP_OK
	}
};