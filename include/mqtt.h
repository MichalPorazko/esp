#pragma once

#include <cstring>


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"



extern const uint8_t hivemq_ca_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t client_crt_pem_start[]     asm("_binary_client_pem_start");
extern const uint8_t client_key_pem_start[]     asm("_binary_client_key_start");





void mqtt_init(void);
void mqtt_start(void* args);