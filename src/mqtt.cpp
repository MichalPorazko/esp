
#include "mqtt.h"





static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, 
    void *event_data);





    
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, 
    void *event_data) {
    
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;                          
    
    switch ((esp_mqtt_event_id_t)event_id) {
        
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI("esp", "Connected to AWS IoT Core");
            xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW("esp", "Disconnected from AWS IoT Core");
            xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI("esp", "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI("esp", "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            xEventGroupSetBits(s_mqtt_event_group, MQTT_PUBLISHED_BIT);
            ESP_LOGI("esp", "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI("esp", "MQTT_EVENT_DATA");

        /** Send immiadiatelly the data to the data base */

            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI("esp", "MQTT_EVENT_ERROR");

            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI("esp", "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI("esp", "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGI("esp", "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI("esp", "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW("esp", "Unknown error type: 0x%x", event->error_handle->error_type);
            }

        break;
        default:
            ESP_LOGD("esp", "MQTT event id: %d", event_id);
            break;
    }
}


void mqtt_init() {
    s_mqtt_event_group = xEventGroupCreate();
    MQTT_CONNECTED_BIT = BIT0;
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

void mqtt_start() {


    
    esp_mqtt_client_register_event(s_mqtt_client, MQTT_EVENT_CONNECTED, mqtt_event_handler, NULL);

    
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));
    ESP_LOGI("esp", "MQTT client started.");
}


void send_mqtt_data(const uint8_t *data, size_t length) {
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