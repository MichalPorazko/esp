#pragma once

#include <cstring>


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"

#define MQTT_TOPIC  "xxx"
#define MQTT_BROKER_URI "xxx"
#define MQTT_PORT 8883
#define MQTT_USERNAME "xxx"
#define MQTT_PASSWORD "xxx"

extern const uint8_t hivemq_ca_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t hivemq_ca_pem_end[]   asm("_binary_isrgrootx1_pem_end");

EventGroupHandle_t s_mqtt_event_group;


esp_mqtt_client_handle_t s_mqtt_client = nullptr;

esp_mqtt_client_config_t mqtt_cfg;

EventBits_t MQTT_CONNECTED_BIT;


void mqtt_start();
void send_mqtt_data(const uint8_t *data, size_t length);