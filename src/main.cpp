#include <cstring>


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"

#include "wifi.h"
#include "mqtt.h"
#include "uart_and_wakeup.h"      




extern "C" void app_main(void) {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_sta();
    uart_init();

    mqtt_start();

    xTaskCreatePinnedToCore(&uart_rx_task, "uart_rx_task", 4096, nullptr, 5, nullptr, 0);    

    config_sleep_mode();

    put_into_light_sleep_mode();
}


