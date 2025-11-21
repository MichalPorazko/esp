#pragma once


extern TaskHandle_t uart_task_handle;
extern TaskHandle_t wifi_task_handle;
extern TaskHandle_t mqtt_task_handle;

uint8_t *buffer;

#define MQTT_TASK_PRIORITY    6
#define WIFI_TASK_PRIORITY    5
#define UART_TASK_PRIORITY   4

#define CORE_0    0
#define CORE_1    1

#define QUEUE_SIZE UART_BUFFER_SIZE