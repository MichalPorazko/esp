#include "task_handles.h"

// Definitions (exactly once in the whole project)
TaskHandle_t uart_task_handle = nullptr;
TaskHandle_t wifi_task_handle = nullptr;
TaskHandle_t mqtt_task_handle = nullptr;

// UART frame buffer used by uart_and_wakeup.cpp etc.
uint8_t buffer[48];
