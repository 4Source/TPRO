#pragma once
#include <cstdint>

// Dummys für FreeRTOS Typen
typedef void *SemaphoreHandle_t;
typedef uint32_t TickType_t;
#define portMAX_DELAY (TickType_t)0xffffffff