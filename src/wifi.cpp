
#include "wifi.h"
#include "mqtt.h"
#include "task_handles.h"
#include "secrets/secrets.h"

esp_event_handler_instance_t wifi_sta_start_handler_instance;
esp_event_handler_instance_t wifi_sta_disconnected_handler_instance;
esp_event_handler_instance_t wifi_sta_got_ip_handler_instance;

 esp_event_loop_handle_t wifi_loop_handle;
 wifi_config_t wifi_config{};
 esp_event_loop_args_t wifi_loop_args = {};


static void wifi_sta_start_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    
    ESP_LOGI("esp", "Wi-Fi station started, connecting…");
    ESP_ERROR_CHECK(esp_wifi_connect());
    
}

static void wifi_sta_disconnected_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    
    ESP_LOGW("esp", "Wi-Fi disconnected/not found, retrying…");
    ESP_ERROR_CHECK(esp_wifi_connect());
    
}

static void wifi_sta_got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    

    vTaskResume(mqtt_task_handle);
    ESP_LOGI("esp", "resumed mqtt task");

}




void wifi_init_sta(void) {

    


    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI("wifi", "ESP-NETIF initialized");


    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI("wifi", "Default event loop created");


    esp_netif_create_default_wifi_sta();
    ESP_LOGI("wifi", "Default Wi-Fi STA created");


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI("wifi", "Wi-Fi initialized");



    wifi_loop_args = {
        .queue_size = 48,
        .task_name = NULL
    };


    
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), WIFI_SSID,
                 sizeof(wifi_config.sta.ssid));
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.password), WIFI_PASS,
                 sizeof(wifi_config.sta.password));


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI("wifi", "Wi-Fi mode set to STA");

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI("wifi", "Wi-Fi configuration set");

    ESP_LOGI("esp", "Wi-Fi initialization complete");

}

void wifi_start(void* args){

    esp_event_loop_create(&wifi_loop_args, &wifi_loop_handle);
    ESP_LOGI("esp", "Wi-Fi event loop created");


    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(wifi_loop_handle, WIFI_EVENT, WIFI_EVENT_STA_START, &wifi_sta_start_handler, nullptr, &wifi_sta_start_handler_instance));
    ESP_LOGI("wifi", "wifi station mode start event handler registered");

    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(wifi_loop_handle, WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_sta_disconnected_handler, nullptr, &wifi_sta_disconnected_handler_instance));
    ESP_LOGI("wifi", "wifi station mode disconnected event handler registered");
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(wifi_loop_handle, IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_got_ip_handler, nullptr, &wifi_sta_got_ip_handler_instance));
    ESP_LOGI("wifi", "wifi station mode got ip event handler registered");

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI("wifi", "Wi-Fi started");

    for (;;)
        vTaskDelay(portMAX_DELAY);
}

void wifi_stop(void){
    ESP_ERROR_CHECK(esp_wifi_stop());

    ESP_LOGI("wifi", "Wi-Fi stopped");

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister_with(wifi_loop_handle, WIFI_EVENT, WIFI_EVENT_STA_START, &wifi_sta_start_handler_instance));
    ESP_LOGI("wifi", "Wi-Fi start event handler unregistered");

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister_with(wifi_loop_handle, WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,  &wifi_sta_disconnected_handler_instance));
    ESP_LOGI("wifi", "Wi-Fi disconnected event handler unregistered");
    
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister_with(wifi_loop_handle, IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_got_ip_handler_instance));
    ESP_LOGI("wifi", "Wi-Fi got IP event handler unregistered");

    esp_event_loop_delete(wifi_loop_handle);
    ESP_LOGI("wifi", "Wi-Fi event loop deleted");
}

