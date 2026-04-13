#pragma once

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_NOT_SUPPORTED 0x106

#ifndef ESP_LOGI
#define ESP_LOGI(tag, format, ...) printf("[INFO ][%s] " format "\n", tag, ##__VA_ARGS__)
#endif

#ifndef ESP_LOGE
#define ESP_LOGE(tag, format, ...) printf("[ERROR][%s] " format "\n", tag, ##__VA_ARGS__)
#endif