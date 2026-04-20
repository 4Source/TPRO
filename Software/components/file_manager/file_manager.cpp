#include "file_manager.hpp"
#include "default_configs.hpp"

#include <dirent.h>
#include <driver/sdspi_host.h>
#include <esp_timer.h>
#include <esp_vfs_fat.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#define SD_PIN_CS GPIO_NUM_4
#define SD_PIN_MISO GPIO_NUM_5
#define SD_PIN_MOSI GPIO_NUM_6
#define SD_PIN_CLK GPIO_NUM_7

sdmmc_card_t *FileManager::card = nullptr;
spi_host_device_t FileManager::host_slot = SPI3_HOST;

std::string FileManager::resolve_path(const std::string &path) {
	if (path.empty() || path == "/") {
		return kMountPoint;
	}
	// If path already starts with the mount point, use it.
	if (path.starts_with(kMountPoint)) {
		return path;
	}
	// Otherwise prepend the mount point safely
	if (path[0] == '/') {
		return std::string(kMountPoint) + path;
	}
	return std::string(kMountPoint) + "/" + path;
}

esp_err_t FileManager::ensure_directories(const std::string &path) {
	std::string temp = path;
	size_t last_slash = temp.find_last_of('/');

	if (last_slash == std::string::npos) {
		return ESP_OK;
	}
	temp = temp.substr(0, last_slash);

	std::string current_path;
	std::stringstream sub_string(temp);
	std::string segment;

	while (std::getline(sub_string, segment, '/')) {
		if (segment.empty()) {
			continue;
		}
		current_path += "/" + segment;

		if (mkdir(current_path.c_str(), 0755) != 0 && errno != EEXIST) {
			ESP_LOGE(kTAG, "mkdir failed: %s (errno: %d)", current_path.c_str(), errno);
			return ESP_FAIL;
		}
	}
	return ESP_OK;
}

esp_err_t FileManager::mount() {

	if (card != nullptr) {
		ESP_LOGW(kTAG, "SD Card is already mounted.");
		return ESP_OK;
	}

	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024ULL,
		.disk_status_check_enable = false,
		.use_one_fat = false,
	};

	ESP_LOGI(kTAG, "Initializing SD card via SPI");

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = SPI3_HOST; // SPI3 nutzen 1&2 werden wohl von ethernet belegt
	host_slot = static_cast<spi_host_device_t>(host.slot);

	// Bus configuration für SPI3
	spi_bus_config_t bus_cfg = {.mosi_io_num = SD_PIN_MOSI,
								.miso_io_num = SD_PIN_MISO,
								.sclk_io_num = SD_PIN_CLK,
								.quadwp_io_num = -1,
								.quadhd_io_num = -1,
								.data4_io_num = -1,
								.data5_io_num = -1,
								.data6_io_num = -1,
								.data7_io_num = -1,
								.data_io_default_level = false,
								.max_transfer_sz = 4092,
								.flags = SPICOMMON_BUSFLAG_MASTER,
								.isr_cpu_id = ESP_INTR_CPU_AFFINITY_0,
								.intr_flags = 0};

	static bool s_spi_bus_initialized = false; // SPI Bus schon von jemandem initialisiert?
	esp_err_t ret = ESP_OK;

	if (!s_spi_bus_initialized) {
		ret = spi_bus_initialize(host_slot, &bus_cfg, SDSPI_DEFAULT_DMA);

		if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
			s_spi_bus_initialized = true;
			ret = ESP_OK;
			ESP_LOGI(kTAG, "SPI bus is ready.");
		} else {
			ESP_LOGE(kTAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
			return ret;
		}
	}

	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = SD_PIN_CS;
	slot_config.host_id = SPI3_HOST;

	ret = esp_vfs_fat_sdspi_mount(kMountPoint, &host, &slot_config, &mount_config, &card);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(kTAG, "Failed to mount filesystem.");
		} else {
			ESP_LOGE(kTAG, "Failed to initialize the card.");
		}
		spi_bus_free(host_slot);
		return ESP_FAIL;
	}

	ESP_LOGI(kTAG, "SDCard mounted at: %s", kMountPoint);
	if (card == nullptr) {
		ESP_LOGW(kTAG, "SD Card is not mounted.");
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t FileManager::unmount() {
	if (card == nullptr) {
		ESP_LOGW(kTAG, "SD Card is not mounted.");
		return ESP_FAIL;
	}
	esp_vfs_fat_sdcard_unmount(kMountPoint, card);
	ESP_LOGI(kTAG, "Card unmounted");
	spi_bus_free(host_slot);
	card = nullptr;
	return ESP_OK;
}

esp_err_t FileManager::save_file(const std::string &filename, const std::string &content) {
	std::string path = resolve_path(filename);

	if (ensure_directories(path) != ESP_OK) {
		ESP_LOGE(kTAG, "Failed to ensure directories for path: %s", path.c_str());
		return ESP_FAIL;
	}

	ESP_LOGI(kTAG, "Saving file: %s", path.c_str());

	std::ofstream file(path);
	if (!file.is_open()) {
		ESP_LOGE(kTAG, "Failed to open file for saving: %s", path.c_str());
		return ESP_FAIL;
	}

	file << content;
	file.close();

	return file.good() ? ESP_OK : ESP_FAIL;
}

esp_err_t FileManager::append_file(const std::string &filename, const std::string &content) {
	std::string path = resolve_path(filename);
	ESP_LOGI(kTAG, "Appending file: %s", path.c_str());

	std::ofstream file(path, std::ios::out | std::ios::app);
	if (!file.is_open()) {
		ESP_LOGE(kTAG, "Failed to open file for appending: %s", path.c_str());
		return ESP_FAIL;
	}

	file << content;
	file.close();

	return file.good() ? ESP_OK : ESP_FAIL;
}

std::optional<std::string> FileManager::read_file(const std::string &filename) {
	std::string path = resolve_path(filename);
	ESP_LOGI(kTAG, "Reading file: %s", path.c_str());

	std::ifstream file(path);
	if (!file.is_open()) {
		ESP_LOGE(kTAG, "Failed to open file for reading: %s", path.c_str());
		return std::nullopt;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	return buffer.str();
}

esp_err_t FileManager::delete_file(const std::string &filename) {
	std::string path = resolve_path(filename);
	ESP_LOGI(kTAG, "Deleting file: %s", path.c_str());

	int result = unlink(path.c_str());
	if (result != 0) {
		ESP_LOGE(kTAG, "Failed to delete file: %s", path.c_str());
		return ESP_FAIL;
	}
	return ESP_OK;
}

std::vector<std::string> FileManager::list_directory(const std::string &directory_path) {
	std::string path = resolve_path(directory_path);
	ESP_LOGI(kTAG, "Listing directory: %s", path.c_str());

	std::vector<std::string> files;
	DIR *dir = opendir(path.c_str());
	if (dir == nullptr) {
		ESP_LOGE(kTAG, "Failed to open directory: %s", path.c_str());
		return files;
	}

	struct dirent *entry = nullptr;
	while ((entry = readdir(dir)) != nullptr) {
		std::string name = static_cast<const char *>(entry->d_name);
		if (name == "." || name == "..") {
			continue;
		}

		if (entry->d_type == DT_DIR) {
			files.push_back(name + "/");
		} else {
			files.push_back(name);
		}
	}

	closedir(dir);
	return files;
}

esp_err_t FileManager::is_file(const std::string &file_path) {
	std::string path = resolve_path(file_path);
	if (path.empty()) {
		ESP_LOGE(kTAG, "Empty path");
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGI(kTAG, "Check is file: %s", path.c_str());

	struct stat status;

	if (stat(path.c_str(), &status) != 0) {
		return ESP_FAIL;
	}

	if (!S_ISREG(status.st_mode)) {
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t FileManager::is_directory(const std::string &directory_path) {
	std::string path = resolve_path(directory_path);
	if (path.empty()) {
		ESP_LOGE(kTAG, "Empty path");
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGI(kTAG, "Check is directory: %s", path.c_str());

	struct stat status;

	if (stat(path.c_str(), &status) != 0) {
		return ESP_FAIL;
	}

	if (!S_ISDIR(status.st_mode)) {
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t FileManager::run_selftest() {
	ESP_LOGI(kTAG, "--- Starting FileManager Selftest ---");
	if (mount() != ESP_OK) {
		ESP_LOGE(kTAG, "Selftest failed: Could not mount SD card.");
		return ESP_FAIL;
	}

	std::string test_file = "selftest.txt";
	std::string test_content = "Hello, SD Card testing!";

	if (save_file(test_file, test_content) != ESP_OK) {
		ESP_LOGE(kTAG, "Selftest failed: Could not save file.");
		return ESP_FAIL;
	}

	if (is_file(test_file) != ESP_OK) {
		ESP_LOGE(kTAG, "Selftest failed: File not found which should exist");
		return ESP_FAIL;
	}

	auto content = read_file(test_file);
	if (!content.has_value() || content.value() != test_content) {
		ESP_LOGE(kTAG, "Selftest failed: Read content does not match.");
		return ESP_FAIL;
	}

	if (is_directory("/") != ESP_OK) {
		ESP_LOGE(kTAG, "Selftest failed: Directory not found which should exist");
		return ESP_FAIL;
	}

	if (is_directory("/test/") == ESP_OK) {
		ESP_LOGE(kTAG, "Selftest failed: Found directory which should not exist");
		return ESP_FAIL;
	}

	auto files = list_directory("/");
	bool found = false;
	for (const auto &file : files) {
		if (file == test_file) {
			found = true;
			break;
		}
	}

	if (!found) {
		ESP_LOGE(kTAG, "Selftest failed: File not found in directory listing.");
		return ESP_FAIL;
	}

	if (delete_file(test_file) != ESP_OK) {
		ESP_LOGE(kTAG, "Selftest failed: Could not delete file.");
		return ESP_FAIL;
	}

	if (is_file(test_file) == ESP_OK) {
		ESP_LOGE(kTAG, "Selftest failed: Found file which should not exist");
		return ESP_FAIL;
	}

	ESP_LOGI(kTAG, "--- FileManager Selftest Passed ---");
	return ESP_OK;
}

esp_err_t FileManager::write_default_configs() {
	// Default configs auf sd karte schreiben
	std::vector<std::pair<std::string, std::string>> default_configs = {
		{DefaultConfigs::kDefaultDayNightConfigPath, DefaultConfigs::kDefaultDayNightConfig},
		{DefaultConfigs::kDefaultBlinkConfigPath, DefaultConfigs::kDefaultBlinkConfig},
		{DefaultConfigs::kDefaultTimelineConfigPath, DefaultConfigs::kDefaultTimelineConfig},
	};

	for (const auto &[path, content] : default_configs) {
		if (save_file(path, content) != ESP_OK) {
			ESP_LOGE(kTAG, "Failed to write default config: %s", path.c_str());
			return ESP_FAIL;
		}
	}
	return ESP_OK;
}

esp_err_t FileManager::print_default_configs() {
	std::vector<std::string> config_paths = {
		DefaultConfigs::kDefaultDayNightConfigPath,
		DefaultConfigs::kDefaultBlinkConfigPath,
		DefaultConfigs::kDefaultTimelineConfigPath,
	};

	for (const auto &path : config_paths) {
		auto content = read_file(path);
		if (!content.has_value()) {
			ESP_LOGE(kTAG, "Failed to read default config: %s", path.c_str());
			return ESP_FAIL;
		}
		ESP_LOGI(kTAG, "Content of %s:\n%s", resolve_path(path).c_str(), content.value().c_str());
	}
	return ESP_OK;
}
