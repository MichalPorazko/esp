
#include "uart_and_wakeup.h"
#include "mqtt.h"
#include "wifi.h"
#include "task_handles.h"

static const char *TAG_WAKEUP = "uart_wakeup";

static QueueHandle_t uart_evt_que = NULL;

 esp_event_loop_handle_t uart_loop_handle;
 esp_event_loop_args_t uart_loop_args = {};


void uart_init(void) {
    uart_config_t uart_config = {};
    uart_config.baud_rate = UART_BAUD_RATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;


    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUFFER_SIZE, 0, QUEUE_SIZE, &uart_evt_que, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_rx_full_threshold(UART_PORT, UART_RX_FULL_THRESH));                 

    
    ESP_LOGI("esp", "UART initialized (port=%d, baud=%d)", UART_PORT, UART_BAUD_RATE);
}


static void uart_data_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
        
    ESP_LOGI("esp", "[UART DATA]: %d", );
    xQueueReceive(uart_evt_que, (void *)buffer, portMAX_DELAY);


}

static void uart_buffer_overflow_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){


    ESP_LOGI("esp", "ring buffer full");
    vTaskResume(uart_task_handle);
}

static void uart_fifo_overflow_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){


    ESP_LOGI("esp", "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
    uart_flush_input(UART_PORT);
    xQueueReset(uart_evt_que);
    free(buffer); 
}

void uart_start(void* args) {


    uart_loop_args.queue_size = QUEUE_SIZE;
    uart_loop_args.task_name = "uart_start";
    uart_loop_args.task_priority = uxTaskPriorityGet(NULL);
    uart_loop_args.task_stack_size = 4 * 1024;
    uart_loop_args.task_core_id = CORE_0;

    esp_event_loop_create(&uart_loop_args, &uart_loop_handle);

    esp_event_handler_instance_register_with(uart_loop_handle, ESP_EVENT_ANY_BASE, UART_DATA, uart_data_handler, NULL, NULL);
    esp_event_handler_instance_register_with(uart_loop_handle, ESP_EVENT_ANY_BASE, UART_BUFFER_FULL, uart_buffer_overflow_handler, NULL, NULL);
    esp_event_handler_instance_register_with(uart_loop_handle, ESP_EVENT_ANY_BASE, UART_FIFO_OVF, uart_fifo_overflow_handler, NULL, NULL);
}


esp_err_t config_sleep_mode(void){
    
    esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_MAX_CPU_FREQ_MHZ, // e.g., 160, 240
        .min_freq_mhz = CONFIG_MIN_CPU_FREQ_MHZ, // e.g., 10, 40, 80
        .light_sleep_enable = true // Enable automatic light sleep
    };
    esp_pm_configure(&pm_config);

    // Configure GPIO wakeup
    ESP_LOGI(TAG_WAKEUP, "Enabling GPIO wakeup on UART ");

    ESP_RETURN_ON_ERROR(gpio_sleep_set_direction(UART_RX_PIN, GPIO_MODE_INPUT), TAG_WAKEUP, "Set uart sleep gpio failed");
    
    ESP_RETURN_ON_ERROR(gpio_sleep_set_pull_mode(UART_RX_PIN, GPIO_PULLUP_ONLY), TAG_WAKEUP, "Set uart sleep gpio failed");
    
    
    
    esp_sleep_enable_uart_wakeup(UART_PORT);
    uart_set_wakeup_threshold(UART_PORT, 3);

    ESP_LOGI(TAG_WAKEUP, "RTC UART configured for wakeup.");
    return ESP_OK;


}


void put_into_light_sleep_mode(void){

    ESP_LOGI("uart_wakeup", "Entering deep sleep now...");
    // Add a small delay to ensure log message is printed before sleep
    vTaskDelay(pdMS_TO_TICKS(200));

    // --- Enter Deep Sleep ---
    esp_light_sleep_start(); 
}


