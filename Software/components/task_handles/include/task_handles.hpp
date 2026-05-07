#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// @brief Alle TaskHandles werden hier verwaltet!
namespace TaskHandle {

constexpr uint32_t kPrioLedController = 6;
constexpr uint32_t kStackSizeLedController = 1024 * 4;
constexpr uint32_t kStackCoreLedController = 1; // Core 1

constexpr uint32_t kPrioWebsocket = 5;
constexpr uint32_t kStackSizeWebSocket = 1024 * 4;

constexpr uint32_t kPrioLightEffectManager = 4;
constexpr uint32_t kStackSizeLightEffectManager = 1024 * 16;
constexpr uint32_t kStackCoreLightEffectManager = 1; // Core 1

// inline verhindert mehrfachdefinitionen
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
inline TaskHandle_t x_light_effect_manager_task_handle = nullptr;
inline TaskHandle_t x_led_controller_task_handle = nullptr;
inline TaskHandle_t x_websocket_task_handle = nullptr;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
} // namespace TaskHandle
