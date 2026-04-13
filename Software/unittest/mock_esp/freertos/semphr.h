#pragma once
#include "FreeRTOS.h"

inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
inline void vSemaphoreDelete(SemaphoreHandle_t x) {}
inline int xSemaphoreTake(SemaphoreHandle_t x, TickType_t t) { return 1; }
inline int xSemaphoreGive(SemaphoreHandle_t x) { return 1; }