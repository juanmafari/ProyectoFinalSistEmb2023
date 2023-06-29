#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with the config you want
   - ie #define EXAMPLE_ESP_WIFI_SSID "mywifissid" */

#define EXAMPLE_ESP_WIFI_SSID_STA      "robasta"
#define EXAMPLE_ESP_WIFI_PASS_STA      "roballostabile"
#define EXAMPLE_ESP_MAXIMUM_RETRY_STA  20

#define EXAMPLE_ESP_WIFI_SSID_AP      "kaluga_wifi"
#define EXAMPLE_ESP_WIFI_PASS_AP      "kalugaucu1"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG_STA = "wifi station";
static const char *TAG_AP = "wifi softAP";

static int s_retry_num_sta = 0;

static void event_handler_sta(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num_sta < EXAMPLE_ESP_MAXIMUM_RETRY_STA) {
            esp_wifi_connect();
            s_retry_num_sta++;
            ESP_LOGI(TAG_STA, "RETRY TO CONNECT TO THE AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG_STA,"CONNECT TO THE AP FAIL");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_STA, "GOT IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num_sta = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


static void event_handler_ap(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG_AP, "STATION "MACSTR" JOIN, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG_AP, "STATION "MACSTR" LEAVE, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_sta, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_sta, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID_STA,
            .password = EXAMPLE_ESP_WIFI_PASS_STA,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_STA, "WIFI_INIT_STA FINISHED.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_STA, "CONNECTED TO AP SSID:%s PASSWORD:%s", EXAMPLE_ESP_WIFI_SSID_STA, EXAMPLE_ESP_WIFI_PASS_STA);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG_STA, "FAILED TO CONNECT TO AP SSID:%s, PASSWORD:%s", EXAMPLE_ESP_WIFI_SSID_STA, EXAMPLE_ESP_WIFI_PASS_STA);
    } else {
        ESP_LOGE(TAG_STA, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

void wifi_init_softap(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_ap, NULL, &instance_any_id));

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID_AP,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID_AP),
            .channel = 2,
            .password = EXAMPLE_ESP_WIFI_PASS_AP,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS_AP) == 0) {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));

    ESP_LOGI(TAG_AP, "WIFI_INIT_SOFTAP FINISHED. SSID:%s PASSWORD:%s", EXAMPLE_ESP_WIFI_SSID_AP, EXAMPLE_ESP_WIFI_PASS_AP);

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
}
