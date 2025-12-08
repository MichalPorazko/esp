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
    ESP_LOGI("MAIN", "NVS flash initialized with return code: %d", ret);

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_LOGI("MAIN", "Allocated UART buffer at %p", buffer);


    uart_init();
    ESP_LOGI("MAIN", "UART initialized");

    wifi_init_sta();
    ESP_LOGI("MAIN", "WiFi initialized");

    mqtt_init();
    ESP_LOGI("MAIN", "MQTT initialized");
    

    xTaskCreatePinnedToCore(uart_rx_task, "uart_rx_task", 2048, nullptr, UART_TASK_PRIORITY, &uart_task_handle, CORE_0); 
    ESP_LOGI("MAIN", "UART task created");

    xTaskCreatePinnedToCore(wifi_start, "wifi_start", 4096, nullptr, WIFI_TASK_PRIORITY, &wifi_task_handle, CORE_1); 
    ESP_LOGI("MAIN", "WiFi task created");

    xTaskCreatePinnedToCore(mqtt_start, "mqtt_start", 4096, nullptr, MQTT_TASK_PRIORITY, &mqtt_task_handle, CORE_1); 
    ESP_LOGI("MAIN", "MQTT task created");   

    config_sleep_mode();
    ESP_LOGI("MAIN", "Configured light sleep mode");

    put_into_light_sleep_mode();
    ESP_LOGI("MAIN", "Device is now in light sleep mode");
}


