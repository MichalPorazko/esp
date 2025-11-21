
#include "mqtt.h"
#include "uart_and_wakeup.h"
#include "wifi.h"
#include "secrets/secrets.h"
#include "task_handles.h"

static void mqtt_stop(void);
static void send_mqtt_data(const uint8_t *data, size_t length) {
    if (s_mqtt_client == nullptr) {
        ESP_LOGW("esp", "MQTT client not ready, dropping UART payload");
        return;
    }

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, MQTT_TOPIC,
                                         reinterpret_cast<const char *>(data),
                                         static_cast<int>(length), 1, 0);
    if (msg_id >= 0) {
        ESP_LOGI("esp", "Published UART payload to HiveMQ (msg_id=%d, len=%d)", msg_id,
                 static_cast<int>(length));
    } else {
        ESP_LOGE("esp", "Failed to publish UART payload (err=%d)", msg_id);
    }
}




void mqtt_event_connected_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {

    ESP_LOGI("esp", "MQTT_EVENT_CONNECTED");
    send_mqtt_data(buffer, (sizeof(*buffer)/sizeof(uint8_t)));

}

void mqtt_event_published_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {

    wifi_stop();
    vTaskSuspend(wifi_task_handle);

    mqtt_stop();
    vTaskSuspend(mqtt_task_handle);

    put_into_light_sleep_mode();

}


    


void mqtt_init(void) {
    

    mqtt_cfg = {};
    // Set the broker URI and other fields via assignments (avoid chained designated initializers)
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;
    
    mqtt_cfg.credentials.client_id = "esp32_client_001"; 
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.broker.address.port = MQTT_PORT;
    mqtt_cfg.broker.verification.certificate = (const char*)hivemq_ca_pem_start;
    mqtt_cfg.broker.verification.certificate_len = hivemq_ca_pem_end - hivemq_ca_pem_start;

    // Initialize the MQTT client with the configuration and event handler
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE("esp", "Failed to initialize MQTT client");
        return;
    }


}

void mqtt_start(void* args) {


    
    esp_mqtt_client_register_event(s_mqtt_client, MQTT_EVENT_CONNECTED, mqtt_event_connected_handler, NULL);
    esp_mqtt_client_register_event(s_mqtt_client, MQTT_EVENT_PUBLISHED, mqtt_event_published_handler, NULL);

    
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));
    ESP_LOGI("esp", "MQTT client started.");
}

static void mqtt_stop(void) {

    esp_mqtt_client_stop(s_mqtt_client);

    esp_mqtt_client_disconnect(s_mqtt_client);

    
    esp_mqtt_client_unregister_event(s_mqtt_client, MQTT_EVENT_CONNECTED, mqtt_event_connected_handler);
    esp_mqtt_client_unregister_event(s_mqtt_client, MQTT_EVENT_PUBLISHED, mqtt_event_published_handler);

    ESP_LOGI("esp", "MQTT client stopped.");
}


