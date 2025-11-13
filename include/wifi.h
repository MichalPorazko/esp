#pragma once

#include <cstring>

#include "esp_err.h"


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_tls.h"
#include "esp_check.h"

#define WIFI_SSID      "xxx"
#define WIFI_PASS      "xxx"

extern TaskHandle_t wifi_task_handle;
extern EventGroupHandle_t s_wifi_event_group;
extern esp_event_handler_instance_t instance_any_id;
extern esp_event_handler_instance_t instance_got_ip;

EventBits_t WIFI_CONNECTED_BIT;

void wifi_init_sta(void);



