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


void wifi_init_sta(void);
void wifi_start(void* args);
void wifi_stop(void);



