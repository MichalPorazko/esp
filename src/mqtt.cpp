
#include "mqtt.h"
#include "uart_and_wakeup.h"
#include "wifi.h"
#include "secrets/secrets.h"
#include "task_handles.h"

esp_mqtt_client_config_t mqtt_cfg = {};
esp_mqtt_client_handle_t s_mqtt_client = nullptr;

static void mqtt_stop(void);
static void send_mqtt_data(const uint8_t *data, size_t length) {
    if (s_mqtt_client == nullptr) {
        ESP_LOGW("mqtt", "MQTT client not ready, dropping UART payload");
        return;
    }


    ESP_LOGI("mqtt", "Sending data");
        for (int i=0;i < (sizeof (*data)/sizeof (data[0]));i++) {
            printf(" %d ",data[i]);
    }


    int msg_id = esp_mqtt_client_publish(s_mqtt_client, MQTT_TOPIC,
                                         reinterpret_cast<const char *>(data),
                                         static_cast<int>(length), 1, 0);
    if (msg_id >= 0) {
        ESP_LOGI("mqtt", "Published UART payload to HiveMQ (msg_id=%d, len=%d)", msg_id,
                 static_cast<int>(length));
    } else {
        ESP_LOGE("mqtt", "Failed to publish UART payload (err=%d)", msg_id);
    }
}




void mqtt_event_connected_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {

    send_mqtt_data(buffer, (sizeof(*buffer)/sizeof(uint8_t)));
    ESP_LOGI("mqtt", "data sent from mqtt_event_connected_handler.");

}

void mqtt_event_published_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {

    wifi_stop();
    ESP_LOGI("mqtt", "WiFi stopped after publishing MQTT message.");

    vTaskSuspend(wifi_task_handle);
    ESP_LOGI("esp", "WiFi task suspended.");

    mqtt_stop();
    ESP_LOGI("mqtt", "MQTT client stopped.");

    vTaskSuspend(mqtt_task_handle);
    ESP_LOGI("esp", "MQTT task suspended.");

    put_into_light_sleep_mode();
    ESP_LOGI("sleep mode", "Device put into light sleep mode.");

}


    


void mqtt_init(void) {
    

 //   mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;
    mqtt_cfg.broker.address.port = MQTT_PORT;
    mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_SSL;
    mqtt_cfg.broker.address.hostname = MQTT_BROKER_URI;
    
    
    mqtt_cfg.broker.address.port = MQTT_PORT;
    mqtt_cfg.broker.verification.certificate = (const char*)root_ca_pem_start;

    mqtt_cfg.credentials.authentication.certificate = (const char*)client_crt_pem_start;
    mqtt_cfg.credentials.authentication.key = (const char*)client_key_pem_start;
    // mqtt_cfg.credentials.username = MQTT_USERNAME;
    // mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;


    ESP_LOGI("mqtt", "MQTT configuration initialized.");

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE("esp", "Failed to initialize MQTT client");
        return;
    }


}

void mqtt_start(void* args) {


    
    esp_mqtt_client_register_event(s_mqtt_client, MQTT_EVENT_CONNECTED, mqtt_event_connected_handler, s_mqtt_client);
    ESP_LOGI("mqtt", "Registered MQTT connected event handler.");
    esp_mqtt_client_register_event(s_mqtt_client, MQTT_EVENT_PUBLISHED, mqtt_event_published_handler, s_mqtt_client);
    ESP_LOGI("mqtt", "Registered MQTT published event handler.");
    
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));
    ESP_LOGI("mqtt", "MQTT client started.");

    for (;;)
        vTaskDelay(portMAX_DELAY);
}

static void mqtt_stop(void) {

    esp_mqtt_client_stop(s_mqtt_client);
    ESP_LOGI("mqtt", "MQTT client stopping...");

    esp_mqtt_client_disconnect(s_mqtt_client);
    ESP_LOGI("mqtt", "MQTT client disconnected.");
    
    esp_mqtt_client_unregister_event(s_mqtt_client, MQTT_EVENT_CONNECTED, mqtt_event_connected_handler);
    ESP_LOGI("mqtt", "Unregistered MQTT connected event handler.");

    esp_mqtt_client_unregister_event(s_mqtt_client, MQTT_EVENT_PUBLISHED, mqtt_event_published_handler);
    ESP_LOGI("mqtt", "Unregistered MQTT published event handler.");
}


