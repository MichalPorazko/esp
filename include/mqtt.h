#pragma once

#include <cstring>


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"



extern const uint8_t hivemq_ca_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t hivemq_ca_pem_end[]   asm("_binary_isrgrootx1_pem_end");



esp_mqtt_client_config_t mqtt_cfg;
esp_mqtt_client_handle_t s_mqtt_client;



void mqtt_init(void);
void mqtt_start(void* args);