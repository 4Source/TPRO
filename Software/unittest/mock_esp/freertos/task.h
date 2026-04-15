#pragma once

#include <cstdint>
typedef void *TaskHandle_t;
typedef uint32_t StackType_t;
typedef uint32_t TickType_t;
typedef long BaseType_t;

#include "esp_additions/freertos_tasks_c_additions.h"

#define pdTRUE (1L)
#define pdFALSE (0L)
#define pdPASS (1L)
#define pdFAIL (0L)
#ifndef portMAX_DELAY
#define portMAX_DELAY (0xFFFFFFFFUL)
#endif

#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(xTimeInMs))

#ifdef __cplusplus
extern "C" {
#endif

inline BaseType_t xTaskCreate(void (*pxTaskCode)(void *), const char *const pcName, const uint32_t usStackDepth, void *const pvParameters,
							  unsigned long uxPriority, TaskHandle_t *const pxCreatedTask) {
	if (pxCreatedTask) {
		*pxCreatedTask = (TaskHandle_t)0x4242; // Dummy Handle für Validierungen
	}
	return pdPASS;
}

inline void vTaskDelete(TaskHandle_t xTaskToDelete) {}

inline void vTaskDelay(const TickType_t xTicksToDelay) {
	// Optional: std::this_thread::sleep_for(std::chrono::milliseconds(xTicksToDelay));
}

inline BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify) {
	// Fake Erfolg
	return pdTRUE;
}

inline uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait) { return 1; }

#ifdef __cplusplus
}
#endif
