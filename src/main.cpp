#include <cstring>

#include "driver/gpio.h"
#include "driver/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "esp_tls.h"



#define WIFI_SSID      "xxx"
#define WIFI_PASS      "xxx"
#define MQTT_TOPIC  "xxx"
#define MQTT_BROKER_URI "xxx" 
#define MQTT_PORT 8883
#define MQTT_USERNAME "xxx"
#define MQTT_PASSWORD "xxx"


extern const uint8_t hivemq_ca_pem_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t hivemq_ca_pem_end[]   asm("_binary_isrgrootx1_pem_end");

static const char *TAG = "esp";




#define UART_PORT           UART_NUM_1
#define UART_TX_PIN         GPIO_NUM_17
#define UART_RX_PIN         GPIO_NUM_16
#define UART_BAUD_RATE      115200
#define UART_BUFFER_SIZE    256




static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t s_mqtt_event_group;


static esp_mqtt_client_handle_t s_mqtt_client = nullptr;


static constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;
static constexpr EventBits_t MQTT_CONNECTED_BIT = BIT0;



static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi station started, connecting…");
        ESP_ERROR_CHECK(esp_wifi_connect());

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected/not found, retrying…");
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_ERROR_CHECK(esp_wifi_connect());

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) (event_data);
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta() {

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, &instance_got_ip));


    wifi_config_t wifi_config{};
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), WIFI_SSID,
                 sizeof(wifi_config.sta.ssid));
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.password), WIFI_PASS,
                 sizeof(wifi_config.sta.password));


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialization complete");

}




static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;                          
    
    switch ((esp_mqtt_event_id_t)event_id) {
        
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to AWS IoT Core");
            xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from AWS IoT Core");
            xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
            //send here the payload if its possible (publish empty payload)
            msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, nullptr, 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        /** Send immiadiatelly the data to the data base */

            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");

            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }

        break;
        default:
            ESP_LOGD(TAG, "MQTT event id: %d", event_id);
            break;
    }
}


static void mqtt_start() {
    esp_mqtt_client_config_t mqtt_cfg = {};
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
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    esp_mqtt_client_register_event(s_mqtt_client, MQTT_EVENT_CONNECTED, mqtt_event_handler, NULL);

    
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));
    ESP_LOGI(TAG, "MQTT client started.");
}


static void uart_init() {
    uart_config_t uart_config = {};
    uart_config.baud_rate = UART_BAUD_RATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUFFER_SIZE * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART initialized (port=%d, baud=%d)", UART_PORT, UART_BAUD_RATE);
}

static void publish_uart_payload(const uint8_t *data, size_t length) {
    if (s_mqtt_client == nullptr) {
        ESP_LOGW(TAG, "MQTT client not ready, dropping UART payload");
        return;
    }

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, MQTT_TOPIC,
                                         reinterpret_cast<const char *>(data),
                                         static_cast<int>(length), 1, 0);
    if (msg_id >= 0) {
        ESP_LOGI(TAG, "Published UART payload to AWS (msg_id=%d, len=%d)", msg_id,
                 static_cast<int>(length));
    } else {
        ESP_LOGE(TAG, "Failed to publish UART payload (err=%d)", msg_id);
    }
}

static void uart_rx_task(void *param) {
    auto *buffer = static_cast<uint8_t *>(pvPortMalloc(UART_BUFFER_SIZE));
    if (buffer == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate UART buffer");
        vTaskDelete(nullptr);
        return;
    }

    while (true) {
        const int len = uart_read_bytes(UART_PORT, buffer, UART_BUFFER_SIZE - 1,
                                        pdMS_TO_TICKS(1000));
        if (len > 0) {
            buffer[len] = '\0';
            ESP_LOGI(TAG, "Received %d bytes from STM32", len);
            xEventGroupWaitBits(s_mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE,
                                portMAX_DELAY);
            publish_uart_payload(buffer, len);
        }
    }
}



extern "C" void app_main(void) {
    

    s_wifi_event_group = xEventGroupCreate();
    s_mqtt_event_group = xEventGroupCreate();

    wifi_init_sta();
    uart_init();
    ESP_LOGI(TAG, "Waiting for Wi-Fi connection…");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE,
                        portMAX_DELAY);

    mqtt_start();

    xTaskCreate(&uart_rx_task, "uart_rx_task", 4096, nullptr, 5, nullptr);
}


