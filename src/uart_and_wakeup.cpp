
#include "uart_and_wakeup.h"
#include "mqtt.h"
#include "wifi.h"
#include "task_handles.h"
#include "http_parser.h"


static QueueHandle_t uart_evt_que = NULL;

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static size_t  buffer_pos = 0;

void uart_init(void) {
    uart_config_t uart_config = {};
    uart_config.baud_rate = UART_BAUD_RATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;


    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, (SOC_UART_FIFO_LEN+1), 0, QUEUE_SIZE, &uart_evt_que, 0));
    ESP_LOGI("uart", "UART driver installed (port=%d)", UART_PORT);

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_LOGI("uart", "UART parameters configured");

    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI("uart", "UART pins set (TX=%d, RX=%d)", UART_TX_PIN, UART_RX_PIN);    


    ESP_ERROR_CHECK(uart_set_rx_full_threshold(UART_PORT, UART_RX_FULL_THRESH));                 
    ESP_LOGI("uart", "UART RX full threshold set to %d bytes", UART_RX_FULL_THRESH);    
    
    ESP_LOGI("uart", "UART initialized (port=%d, baud=%d)", UART_PORT, UART_BAUD_RATE);
}

void uart_rx_task(void* args) {

    uart_event_t event;
    int len = 0;

    while(1) {

        if (xQueueReceive(uart_evt_que, (void*)&event, (TickType_t)portMAX_DELAY)) {
            ESP_LOGI("uart", "uart%d recved event:%d", UART_PORT, event.type);

            switch (event.type) {
                case UART_DATA:
                    ESP_LOGI("uart", "uart data, len: %d", event.size);

                    len = uart_read_bytes(
                        UART_PORT,
                        buffer + buffer_pos,
                        MIN(event.size, sizeof(buffer) - buffer_pos),
                        0
                    );
                    buffer_pos += len;

                    if (buffer_pos >= UART_RX_FULL_THRESH) {
                        ESP_LOGI("uart", "Received all data, reasuming the Wifi Task");
                        buffer_pos = 0;
                        vTaskResume(wifi_task_handle);
                    }
                    break;

                case UART_FIFO_OVF:
                    ESP_LOGI("uart", "hw fifo overflow");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_evt_que);
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGI("uart", "ring buffer full");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_evt_que);
                    break;

                case UART_BREAK:
                    ESP_LOGI("uart", "uart rx break");
                    break;

                case UART_PARITY_ERR:
                    ESP_LOGI("uart", "uart parity error");
                    break;

                case UART_FRAME_ERR:
                    ESP_LOGI("uart", "uart frame error");
                    break;

    #if SOC_UART_SUPPORT_WAKEUP_INT
                case UART_WAKEUP:
                    ESP_LOGI("uart", "uart wakeup");
                    break;
    #endif

                case UART_DATA_BREAK:
                case UART_PATTERN_DET:
                case UART_EVENT_MAX:
                    break;

                default:
                    ESP_LOGI("uart", "unhandled event: %d", event.type);
                    break;
            }
        }

    }

    vTaskDelete(NULL);
}
esp_err_t config_sleep_mode(void){
    
    esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_MAX_CPU_FREQ_MHZ, // e.g., 160, 240
        .min_freq_mhz = CONFIG_MIN_CPU_FREQ_MHZ, // e.g., 10, 40, 80
        .light_sleep_enable = true // Enable automatic light sleep
    };
    esp_pm_configure(&pm_config);
    ESP_LOGI("wakeup", "Power management configured: max %d MHz, min %d MHz, light sleep %s",
             pm_config.max_freq_mhz,
             pm_config.min_freq_mhz,
             pm_config.light_sleep_enable ? "enabled" : "disabled");


    // Configure GPIO wakeup
    ESP_LOGI("wakeup", "Enabling GPIO wakeup on UART ");

    ESP_RETURN_ON_ERROR(gpio_sleep_set_direction(UART_RX_PIN, GPIO_MODE_INPUT), "wakeup", "Set uart sleep gpio failed");
    
    ESP_RETURN_ON_ERROR(gpio_sleep_set_pull_mode(UART_RX_PIN, GPIO_PULLUP_ONLY), "wakeup", "Set uart sleep gpio failed");
    
    
    
    esp_sleep_enable_uart_wakeup(UART_PORT);
    ESP_LOGI("wakeup", "UART wakeup enabled on UART port %d", UART_PORT);    
    
    uart_set_wakeup_threshold(UART_PORT, 3);
    ESP_LOGI("wakeup", "UART wakeup threshold set to 3 bytes");

    ESP_LOGI("wakeup", "RTC UART configured for wakeup.");
    return ESP_OK;


}


void put_into_light_sleep_mode(void){

    ESP_LOGI("uart_wakeup", "Entering deep sleep now...");
    // Add a small delay to ensure log message is printed before sleep
    vTaskDelay(pdMS_TO_TICKS(200));

    // --- Enter Deep Sleep ---
    esp_light_sleep_start(); 
}