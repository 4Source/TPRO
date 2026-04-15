#pragma once
typedef void *TaskHandle_t;
typedef long BaseType_t;

#define pdPASS (1L)

inline BaseType_t xTaskCreatePinnedToCore(void (*pxTaskCode)(void *), const char *const pcName, const uint32_t usStackDepth, void *const pvParameters,
										  unsigned long uxPriority, TaskHandle_t *const pxCreatedTask, const BaseType_t xCoreID) {
	if (pxCreatedTask) {
		*pxCreatedTask = (TaskHandle_t)0x4242; // Dummy Handle für Validierungen
	}
	return pdPASS;
}