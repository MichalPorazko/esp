
#include "uart_and_wakeup.h"
#include "mqtt.h"

static const char *TAG_WAKEUP = "uart_wakeup";

static QueueHandle_t uart_evt_que = NULL;

void uart_init() {
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

    ESP_LOGI("esp", "UART initialized (port=%d, baud=%d)", UART_PORT, UART_BAUD_RATE);
}



void uart_rx_task(void *param) {
    
    auto *buffer = static_cast<uint8_t *>(pvPortMalloc(UART_BUFFER_SIZE));
    if (buffer == nullptr) {
        ESP_LOGE("esp", "Failed to allocate UART buffer");
        vTaskDelete(nullptr);
        return;
    }

    uart_event_t event;
    if (uart_evt_que == NULL) {
        ESP_LOGE("esp", "uart_evt_que is NULL");
        abort();
    }

    while(1) {
        // Waiting for UART event.
        if(xQueueReceive(uart_evt_que, (void * )&event, (TickType_t)portMAX_DELAY)) {
            
            ESP_LOGI("esp", "uart%d recved event:%d", UART_PORT, event.type);

            switch(event.type) {
                case UART_DATA:
                    
                ESP_LOGI("esp", "[UART DATA]: %d", event.size);
                uart_read_bytes(UART_PORT, buffer, event.size, portMAX_DELAY);
                                        
                xEventGroupWaitBits(s_mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE,
                                       portMAX_DELAY);
                send_mqtt_data(buffer, event.size);
                
                break;        

                // Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI("esp", "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_evt_que);
                    break;
                // Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI("esp", "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_evt_que);
                    break;
                // Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI("esp", "uart rx break");
                    break;
                // Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI("esp", "uart parity error");
                    break;
                // Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI("esp", "uart frame error");
                    break;
                // ESP32 can wakeup by uart but there is no wake up interrupt
#if SOC_UART_SUPPORT_WAKEUP_INT
                // Event of waking up by UART
                case UART_WAKEUP:
                    ESP_LOGI(TAG, "uart wakeup");
                    break;
#endif
                default:
                    ESP_LOGI("esp", "uart event type: %d", event.type);
                    break;
            }
        }
    }
    // free(buffer); ????
    // vTaskDelete(NULL);  ????

}


esp_err_t config_sleep_mode(void){
    
    // esp_pm_config_t pm_config = {
    //     .max_freq_mhz = CONFIG_EXAMPLE_MAX_CPU_FREQ_MHZ, // e.g., 160, 240
    //     .min_freq_mhz = CONFIG_EXAMPLE_MIN_CPU_FREQ_MHZ, // e.g., 10, 40, 80
    //     .light_sleep_enable = true // Enable automatic light sleep
    // };
    // esp_err_t err = esp_pm_configure(&pm_config);

    // Configure GPIO wakeup
    ESP_LOGI(TAG_WAKEUP, "Enabling GPIO wakeup on UART ");

    

    /* UART will wakeup the chip up from light sleep if the edges that RX pin received has reached the threshold
     * Besides, the Rx pin need extra configuration to enable it can work during light sleep */

    
    /*
        GPIO set direction at sleep
 
        Configure GPIO direction,such as output_only,input_only,output_and_input
    */ 
    ESP_RETURN_ON_ERROR(gpio_sleep_set_direction(UART_RX_PIN, GPIO_MODE_INPUT), TAG_WAKEUP, "Set uart sleep gpio failed");
    
    
    /*
        Configure GPIO pull-up/pull-down resistors at sleep
    
        @note ESP32: Only pins that support both input & output have integrated pull-up and pull-down resistors. 
        Input-only GPIOs 34-39 do not.
    */
    ESP_RETURN_ON_ERROR(gpio_sleep_set_pull_mode(UART_RX_PIN, GPIO_PULLUP_ONLY), TAG_WAKEUP, "Set uart sleep gpio failed");
    
    
    
    esp_sleep_enable_uart_wakeup(UART_PORT);
    uart_set_wakeup_threshold(UART_PORT, 3);

    ESP_LOGI(TAG_WAKEUP, "RTC UART configured for wakeup.");
    return ESP_OK;


    ESP_LOGI(TAG_WAKEUP, "Entering deep sleep now...");
    // Add a small delay to ensure log message is printed before sleep
    vTaskDelay(pdMS_TO_TICKS(200));

    // --- Enter Deep Sleep ---
    esp_light_sleep_start(); // This function does not return

}

void wake_up_callback(void) {
    
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_UNDEFINED) { // Skip first boot
        ESP_LOGI(TAG_WAKEUP, "Wakeup cause: %d", cause);

        if (cause == ESP_SLEEP_WAKEUP_UART) {
        
            ESP_LOGI(TAG_WAKEUP, "Woken up by GPIO.");
            
        } else {
            ESP_LOGI(TAG_WAKEUP, "Woken up by other source: %d", cause);
        }
    } else {
        ESP_LOGI(TAG_WAKEUP, "First boot or not woken from deep sleep.");
    }
    
}

