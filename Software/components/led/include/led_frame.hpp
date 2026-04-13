#pragma once

#include <array>
#include <cstdint>
/// @brief Ein RGB-Wert für eine LED - erzwingend 1 Byte pro Farbe für websocket senden als byte array
struct __attribute__((packed)) RGB {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	bool operator==(const RGB &) const = default;
};

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/// @brief Ein Frame, der die LED-Daten für alle LEDs enthält
// Jeder Effect hat einen LED Frame
// Ein LED Frame wird später statisch sein, er wird vom Licht-Effekt Manager
// beschrieben und vom Output Layer gelesen
// INFO: Reader/Writer Lock wird von rtos nicht unterstützt deshalb selbst implemetiert
class LedFrame {
  public:
	static constexpr uint16_t WIDTH = 54;
	static constexpr uint16_t HEIGHT = 60;

	std::array<std::array<RGB, HEIGHT>, WIDTH> led_data;

	LedFrame() {
		m_resource_mutex = xSemaphoreCreateMutex();
		m_reader_mutex = xSemaphoreCreateMutex();
	}
	~LedFrame() {
		if (m_resource_mutex != nullptr) {
			vSemaphoreDelete(m_resource_mutex);
		}
		if (m_reader_mutex != nullptr) {
			vSemaphoreDelete(m_reader_mutex);
		}
	}
	/// @brief Scoped Lock für schreiben
	struct ScopedWriteLock {
		LedFrame &frame;
		explicit ScopedWriteLock(LedFrame &f) : frame(f) { frame.lock_write(); }
		~ScopedWriteLock() { frame.unlock_write(); }

		ScopedWriteLock(const ScopedWriteLock &) = delete;
		ScopedWriteLock &operator=(const ScopedWriteLock &) = delete;
	};

	/// @brief Scoped Lock für lesen
	struct ScopedReadLock {
		LedFrame &frame;
		explicit ScopedReadLock(LedFrame &f) : frame(f) { frame.lock_read(); }
		~ScopedReadLock() { frame.unlock_read(); }

		ScopedReadLock(const ScopedReadLock &) = delete;
		ScopedReadLock &operator=(const ScopedReadLock &) = delete;
	};

  private:
	SemaphoreHandle_t m_resource_mutex; // Schützt Schreiben
	SemaphoreHandle_t m_reader_mutex;	// Schützt Reader Count
	uint8_t m_active_readers = 0;
	// For Writers
	void lock_write() { xSemaphoreTake(m_resource_mutex, portMAX_DELAY); }

	void unlock_write() { xSemaphoreGive(m_resource_mutex); }

	// For Readers
	void lock_read() {
		xSemaphoreTake(m_reader_mutex, portMAX_DELAY);
		m_active_readers++;

		if (m_active_readers == 1) {
			xSemaphoreTake(m_resource_mutex, portMAX_DELAY);
		}
		xSemaphoreGive(m_reader_mutex);
	}

	void unlock_read() {
		xSemaphoreTake(m_reader_mutex, portMAX_DELAY);
		m_active_readers--;
		if (m_active_readers == 0) {
			xSemaphoreGive(m_resource_mutex);
		}
		xSemaphoreGive(m_reader_mutex);
	}
};