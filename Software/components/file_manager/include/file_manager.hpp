#pragma once

#include <esp_err.h>
#include <optional>
#include <string>
#include <vector>

class FileManager {
  public:
	/**
	 * @brief Initialize and mount the SD Card using VFS FatFs.
	 *
	 * @return ESP_OK if successful, ESP_FAIL otherwise.
	 */
	static esp_err_t mount();

	/**
	 * @brief Unmount the SD Card.
	 * @return ESP_OK if successful, ESP_FAIL otherwise.
	 */
	static esp_err_t unmount();

	/**
	 * @brief Save the given content to a file.
	 *
	 * @param filename Name of the file (e.g. "/sdcard/config.json").
	 *                 If it does not start with "/", "/sdcard/" will be prepended.
	 * @param content String to write to the file.
	 * @return ESP_OK if successful, ESP_FAIL otherwise.
	 */
	static esp_err_t save_file(const std::string &filename, const std::string &content);

	/**
	 * @brief Read the contents of a file.
	 *
	 * @param filename File name.
	 * @return std::optional<std::string> String containing file content, or std::nullopt if reading failed.
	 */
	static std::optional<std::string> read_file(const std::string &filename);

	/**
	 * @brief Delete a configuration file.
	 *
	 * @param filename File name.
	 * @return ESP_OK if successful, ESP_FAIL otherwise.
	 */
	static esp_err_t delete_file(const std::string &filename);

	/**
	 * @brief List files in a directory.
	 *
	 * @param directory_path The path to list files from (e.g., "/sdcard").
	 * @return std::vector<std::string> A list of filenames found in the directory.
	 */
	static std::vector<std::string> list_directory(const std::string &directory_path);

	/**
	 * @brief Run an internal test to verify saving, reading, and deleting a file.
	 *        Call this from app_main to test in a real environment.
	 *
	 * @return ESP_OK if all tests pass, ESP_FAIL otherwise.
	 */
	static esp_err_t run_selftest();
};
