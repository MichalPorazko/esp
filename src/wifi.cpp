
#include "wifi.h"




static constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;


static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);






static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {


    ESP_ERROR_CHECK(esp_wifi_start());                            

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI("esp", "Wi-Fi station started, connecting…");
        ESP_ERROR_CHECK(esp_wifi_connect());


    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW("esp", "Wi-Fi disconnected/not found, retrying…");
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_ERROR_CHECK(esp_wifi_connect());

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) (event_data);
        ESP_LOGI("esp", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void) {

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
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
    

    ESP_LOGI("esp", "Wi-Fi initialization complete");

}

