#pragma once

#include <driver/spi_common.h>
#include <esp_err.h>
#include <esp_log.h>
#include <fstream>
#include <functional>
#include <optional>
#include <sdmmc_cmd.h>
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
	 * @brief Append the given content to a file.
	 *
	 * @param filename Name of the file (e.g. "/sdcard/config.json").
	 *                 If it does not start with "/", "/sdcard/" will be prepended.
	 * @param content String to write to the end of the file.
	 * @return ESP_OK if successful, ESP_FAIL otherwise.
	 */
	static esp_err_t append_file(const std::string &filename, const std::string &content);

	/**
	 * @brief Read the content of a file.
	 *
	 * @param filename File name.
	 * @return std::optional<std::string> String containing file content, or std::nullopt if reading failed.
	 */
	static std::optional<std::string> read_file(const std::string &filename);

	/**
	 * @brief Read of the content of a file in chunks passes it to the 'on_chunk' lambda for processing.
	 *
	 * @param filename File name.
	 * @param on_chunk The method which is called to receive the next chunk.
	 * @retval - `ESP_OK`: Successful
	 * @retval - `ESP_FAIL`: When failed to open the file
	 * @retval - `other`: when lambda returns error the error is passed through
	 */
	template <size_t N> static esp_err_t read_file_chunked(const std::string &filename, const std::function<esp_err_t(const char *, int)> &on_chunk) {
		std::string path = resolve_path(filename);
		ESP_LOGI(kTag, "Reading chunked file: %s", path.c_str());

		std::ifstream file(path);
		if (!file.is_open()) {
			ESP_LOGE(kTag, "Failed to open file for reading: %s", path.c_str());
			return ESP_FAIL;
		}

		std::array<char, N> buffer;
		while (file) {
			file.read(buffer.data(), N);
			std::streamsize read_count = file.gcount();
			if (read_count > 0) {
				esp_err_t err = on_chunk(buffer.data(), static_cast<int>(read_count));
				if (err != ESP_OK) {
					return err;
				}
			}
		}

		return ESP_OK;
	}

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
	 * @brief Check if file path points to a file that is existing.
	 *
	 * @param file_path The path to the files to check
	 * @retval - `ESP_OK`: if the path is a file that does exists
	 * @retval - `ESP_FAIL`: if the path is a file that does NOT exists
	 */
	static esp_err_t is_file(const std::string &file_path);

	/**
	 * @brief Check if path is a existing directory.
	 *
	 * @param directory_path The path to check
	 * @retval - `ESP_OK`: if the path is a directory that does exists
	 * @retval - `ESP_FAIL`: if the path is a directory that does NOT exists
	 */
	static esp_err_t is_directory(const std::string &directory_path);

	/**
	 * @brief Run an internal test to verify saving, reading, and deleting a file.
	 *        Call this from app_main to test in a real environment.
	 *
	 * @return ESP_OK if all tests pass, ESP_FAIL otherwise.
	 */
	static esp_err_t run_selftest();

	/**
	 * Wählt eine zufällige .json Datei aus dem angegebenen Verzeichnis aus.
	 * @param directory_path Das Verzeichnis (Standard: "/effects")
	 * @return Den vollständigen Pfad zur zufälligen Datei oder std::nullopt, falls keine gefunden wurde.
	 */
	static std::optional<std::string> get_random_effect_path();

	// 1. Buffer-basiert (für Binärdaten, Firmware-Updates, Netzwerk-Chunks)
	static esp_err_t write_buffer(const std::string &filename, const uint8_t *data, size_t len, bool append = true);
	static esp_err_t read_buffer_chunk(const std::string &filename, uint8_t *out_dest, size_t offset, size_t len, size_t *bytes_read);

	// 2. Stream-basiert (für ArduinoJson & zeilenweises Einlesen)
	static FILE *open_file(const std::string &filename, const char *mode);
	static void close_file(FILE *f);
	static bool read_line(FILE *f, char *buffer, size_t max_len);

  private:
	/**
	 * @brief Ensures the path starts with the mount point
	 *
	 * @param path The path to use
	 * @return The file path on the SD Card
	 */
	static std::string resolve_path(const std::string &path);

	static constexpr const char *kTag = "file-manager";
	static constexpr const char *kMountPoint = "/sdcard";
	static sdmmc_card_t *card;
	static spi_host_device_t host_slot;

	static esp_err_t ensure_directories(const std::string &path);
};
