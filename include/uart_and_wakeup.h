
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




#define UART_PORT           UART_NUM_0
#define UART_TX_PIN         GPIO_NUM_1
#define UART_RX_PIN         GPIO_NUM_3
#define UART_BAUD_RATE      115200
#define UART_BUFFER_SIZE    48
#define UART_WAKEUP_TRESHOLD     3  
#define UART_RX_FULL_THRESH        48
#define CONFIG_MAX_CPU_FREQ_MHZ 200
#define CONFIG_MIN_CPU_FREQ_MHZ 60




void uart_init(void);
void uart_rx_task(void* args);

esp_err_t config_sleep_mode(void);

void put_into_light_sleep_mode(void);




