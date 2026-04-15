#pragma once

class FileManager {
  public:
	// Wichtig: Der Name muss mock_content sein, damit dein Test-Code darauf zugreifen kann
	static inline const char *mock_content = nullptr;

	static const char *read_file(const char *path) { return mock_content; }
};