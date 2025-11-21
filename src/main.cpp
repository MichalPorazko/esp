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
#include "task_handles.h"    




extern "C" void app_main(void) {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    buffer = static_cast<uint8_t *>(pvPortMalloc(UART_BUFFER_SIZE));

    uart_init();
    wifi_init_sta();
    mqtt_init();
    

    xTaskCreatePinnedToCore(uart_start, "uart_start", 2048, nullptr, UART_TASK_PRIORITY, &uart_task_handle, CORE_0); 
    xTaskCreatePinnedToCore(wifi_start, "wifi_start", 4096, nullptr, WIFI_TASK_PRIORITY, &wifi_task_handle, CORE_1); 
    xTaskCreatePinnedToCore(mqtt_start, "mqtt_start", 4096, nullptr, MQTT_TASK_PRIORITY, &mqtt_task_handle, CORE_1);    

    config_sleep_mode();

    put_into_light_sleep_mode();
}


