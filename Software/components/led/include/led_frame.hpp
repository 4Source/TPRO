#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string>

// This is for clang tidy when not configured the files still get analyzed and than have missing defines
#ifdef __clang__
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#ifndef CONFIG_LED_COLUMNS
#define CONFIG_LED_COLUMNS 87
#endif
#ifndef CONFIG_LED_CH1_ROWS
#define CONFIG_LED_CH1_ROWS 12
#endif
#ifndef CONFIG_LED_CH2_ROWS
#define CONFIG_LED_CH2_ROWS 11
#endif
#ifndef CONFIG_LED_CH3_ROWS
#define CONFIG_LED_CH3_ROWS 12
#endif
#ifndef CONFIG_LED_STRIP_RESOLUTION_HZ
#define CONFIG_LED_STRIP_RESOLUTION_HZ 10000000
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)
#endif
/// @brief Ein RGB-Wert für eine LED - erzwingend 1 Byte pro Farbe für websocket senden als byte array
struct __attribute__((packed)) RGB {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	bool operator==(const RGB &) const = default;
	constexpr RGB &operator*=(float rhs) {
		this->red = static_cast<uint8_t>(std::clamp<float>(this->red * rhs, 0.0f, 255.0f));
		this->green = static_cast<uint8_t>(std::clamp<float>(this->green * rhs, 0.0f, 255.0f));
		this->blue = static_cast<uint8_t>(std::clamp<float>(this->blue * rhs, 0.0f, 255.0f));
		return *this;
	}
	constexpr RGB &operator+=(const RGB &rhs) {
		this->red = static_cast<uint8_t>(std::clamp<int>(this->red + rhs.red, 0, 255));
		this->green = static_cast<uint8_t>(std::clamp<int>(this->green + rhs.green, 0, 255));
		this->blue = static_cast<uint8_t>(std::clamp<int>(this->blue + rhs.blue, 0, 255));
		return *this;
	}
};

constexpr RGB operator*(RGB lhs, float rhs) { return lhs *= rhs; };
constexpr RGB operator+(RGB lhs, RGB rhs) { return lhs += rhs; };

/// @brief Ein Frame, der die LED-Daten für alle LEDs enthält
// Jeder Effect hat einen LED Frame
// Ein LED Frame wird später statisch sein, er wird vom Licht-Effekt Manager
// beschrieben und vom Output Layer gelesen
// INFO: Reader/Writer Lock wird von rtos nicht unterstützt deshalb selbst implemetiert
class LedFrame {
  public:
	// NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
	std::array<std::array<RGB, CONFIG_LED_COLUMNS>, CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS + CONFIG_LED_CH3_ROWS> led_data;

	LedFrame();
	~LedFrame();
	// Nur explizit kopieren erlaubt
	LedFrame(const LedFrame &) = delete;
	LedFrame &operator=(const LedFrame &) = delete;
	LedFrame(LedFrame &&) = delete;
	LedFrame &operator=(LedFrame &&) = delete;

	void copy_data_from(const LedFrame &other);
	uint32_t hash() const {
		// FNV-1a 32-bit
		uint32_t hash = 2166136261u;
		constexpr uint32_t prime = 16777619u;

		const uint8_t *data_ptr = reinterpret_cast<const uint8_t *>(led_data.data());

		constexpr size_t total_bytes = sizeof(led_data);

		for (size_t i = 0; i < total_bytes; i++) {
			hash ^= data_ptr[i];
			hash *= prime;
		}

		return hash;
	}

	/// @brief Scoped Lock für schreiben
	struct ScopedWriteLock {
		LedFrame &frame;
		explicit ScopedWriteLock(LedFrame &frame) : frame(frame) { this->frame.lock_write(); }
		~ScopedWriteLock() { frame.unlock_write(); }

		ScopedWriteLock(const ScopedWriteLock &) = delete;
		ScopedWriteLock &operator=(const ScopedWriteLock &) = delete;
		ScopedWriteLock(ScopedWriteLock &&) = delete;
		ScopedWriteLock &operator=(ScopedWriteLock &&) = delete;
	};

	/// @brief Scoped Lock für lesen
	struct ScopedReadLock {
		LedFrame &frame;
		explicit ScopedReadLock(LedFrame &frame) : frame(frame) { this->frame.lock_read(); }
		~ScopedReadLock() { frame.unlock_read(); }

		ScopedReadLock(const ScopedReadLock &) = delete;
		ScopedReadLock &operator=(const ScopedReadLock &) = delete;
		ScopedReadLock(ScopedReadLock &&) = delete;
		ScopedReadLock &operator=(ScopedReadLock &&) = delete;
	};

	void print_led_data_csv_table();
	void clear_led_data();

  private:
	static constexpr const char *kTag = "led-frame";

	SemaphoreHandle_t m_resource_mutex; // Schützt Schreiben
	SemaphoreHandle_t m_reader_mutex;	// Schützt Reader Count
	uint8_t m_active_readers = 0;
	// For Writers
	void lock_write();
	void unlock_write();

	// For Readers
	void lock_read();
	void unlock_read();
};