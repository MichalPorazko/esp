
#pragma once

#include <cstring>

#include "driver/uart.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_check.h"

#include <time.h> 
#include "esp_sleep.h" 
#include "esp_pm.h"     


#define UART_PORT           UART_NUM_1
#define UART_TX_PIN         GPIO_NUM_17
#define UART_RX_PIN         GPIO_NUM_16
#define UART_BAUD_RATE      115200
#define UART_BUFFER_SIZE    48
#define UART_WAKEUP_TRESHOLD     3  
#define UART_RX_FULL_THRESH        48


TaskHandle_t s_uart_rx_task_handle;


void uart_rx_task(void *param);
void uart_init();

esp_err_t config_sleep_mode(void);

void put_into_light_sleep_mode(void);


