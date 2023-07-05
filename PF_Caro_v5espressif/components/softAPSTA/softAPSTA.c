#include <stdio.h>
#include "softAPSTA.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
    #include "nvs_flash.h"
#include "nvs.h" //NUEVA BIBLIOTECA
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include <esp_http_server.h>
/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with the config you want
   - ie #define EXAMPLE_ESP_WIFI_SSID "mywifissid" */

#define EXAMPLE_ESP_WIFI_SSID_STA      ""
#define EXAMPLE_ESP_WIFI_PASS_STA      ""


#define EXAMPLE_ESP_WIFI_SSID_AP      "kaluga_caro"
#define EXAMPLE_ESP_WIFI_PASS_AP      "kalugaucu1"

#define EXAMPLE_ESP_MAXIMUM_RETRY  20
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG_STA = "wifi station";
static const char *TAG_AP = "wifi softAP";

static int s_retry_num = 0;
/* FreeRTOS event group to signal when we are connected/disconnected */
static EventGroupHandle_t s_wifi_event_group;
esp_netif_t *esp_netif_ap;


/*HTML */

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
char usuario[32] = "";
char password[32] = "";

char g_ssid[32] = "";
char g_password[64] = "";
//char g_mqtturl[64] = "";
//char g_mqttport[32] = "";

char* ssidPr;
char* passPr;

int connectCheck = 0;

void init_sta(void *pvParameters);
void save_credentials_to_nvs();

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG_STA, "Station started");
        connectCheck = 1;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_netif_t *wifi_init_softap(void)
{
    esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID_AP,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID_AP),
            .channel = 2,
            .password = EXAMPLE_ESP_WIFI_PASS_AP,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_PASS_AP) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID_AP, EXAMPLE_ESP_WIFI_PASS_AP, 2);

    return esp_netif_ap;
}

/* Initialize wifi station */
esp_netif_t *wifi_init_sta(void)
{
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = EXAMPLE_ESP_MAXIMUM_RETRY,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
            * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strncpy((char*)wifi_sta_config.sta.ssid, g_ssid, sizeof(wifi_sta_config.sta.ssid));
    strncpy((char*)wifi_sta_config.sta.password, g_password, sizeof(wifi_sta_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config) );

    ESP_LOGI(TAG_STA, "wifi_init_sta finished.");

    return esp_netif_sta;
}

esp_err_t root_get_handler(httpd_req_t *req)
{
    extern const uint8_t web_html_start[] asm("_binary_web_html_start");
    extern const uint8_t web_html_end[] asm("_binary_web_html_end");

    httpd_resp_send(req, (const char *)web_html_start, web_html_end - web_html_start);

    // Retorna ESP_OK si la solicitud se maneja correctamente
    return ESP_OK;
}

esp_err_t config_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Payload Too Large");
        return ESP_FAIL;
    }

    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }

        buf[ret] = '\0';  // Agregar terminador nulo al final del búfer

        if (httpd_query_key_value(buf, "ssid", g_ssid, sizeof(g_ssid)) == ESP_OK &&
            httpd_query_key_value(buf, "password", g_password, sizeof(g_password)) == ESP_OK
            //&& httpd_query_key_value(buf, "mqqt_url", g_mqtturl, sizeof(g_mqtturl)) == ESP_OK
           // && httpd_query_key_value(buf, "mqtt_pott", g_mqttport, sizeof(g_mqttport)) == ESP_OK
            ) {

            save_credentials_to_nvs();
             
        } else {
            return ESP_FAIL;  // Datos del formulario incompletos o incorrectos
        }

        remaining -= ret;
    }

    httpd_resp_send(req, "Datos recibidos", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_handle_t start_webserver()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t config_post = {
            .uri = "/config",
            .method = HTTP_POST,
            .handler = config_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &config_post);
    }

    return server;
}

void stop_webserver(httpd_handle_t server)
{
    if (server) {
        httpd_stop(server);
    }
}

void init_ap(void *pvParameters){

    while (1) {
        if (strlen(g_ssid) > 0 && strlen(g_password) > 0) {
        
        TaskHandle_t task_handle;
        xTaskCreate(init_sta, "init_sta_task", 1024*2, NULL, 1, &task_handle);
        vTaskDelete(NULL); // Eliminar la tarea init_ap
        }
        
     vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void init_sta(void *pvParameters) {

    while (1) {
        if (strlen(g_ssid) > 0 && strlen(g_password) > 0) {
            printf("usuario: %s  ,contraseña: %s \n",g_ssid,g_password );
            ESP_ERROR_CHECK(esp_wifi_stop() );
           
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));


            ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
            esp_netif_t *esp_netif_sta = wifi_init_sta();

            ESP_ERROR_CHECK(esp_wifi_start());
            vTaskDelete(NULL);

        }
    
    }
}


int softapsta(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize event group */
    s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler,
                    NULL,
                    NULL));

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    /* Initialize AP */
    ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
    esp_netif_t *esp_netif_ap = wifi_init_softap();

    
    /* Initialize STA */
    TaskHandle_t task_handle;
    xTaskCreate(init_ap, "init_ap_task", configMINIMAL_STACK_SIZE, NULL, 1, &task_handle);

    /* Start WiFi */
    

    ESP_ERROR_CHECK(esp_wifi_start() );

    /* Set sta as the default interface */
    //esp_netif_set_default_netif(esp_netif_sta);

    /* Enable napt on the AP netif */
    /*if (esp_netif_napt_enable(esp_netif_ap) != ESP_OK) {
        ESP_LOGE(TAG_STA, "NAPT not enabled on the netif: %p", esp_netif_ap);
    }*/
    start_webserver();
    while(1){
        if (connectCheck == 1)
        {
            return connectCheck;
        }
        
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
    
}

void save_credentials_to_nvs() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS handle!\n");
        return;
    }

    err = nvs_set_str(nvs_handle, "ssid", g_ssid);
    if (err != ESP_OK) {
        printf("Error setting SSID to NVS!\n");
        return;
    }

    err = nvs_set_str(nvs_handle, "password", g_password);
    if (err != ESP_OK) {
        printf("Error setting password to NVS!\n");
        return;
    }

    // err = nvs_set_str(nvs_handle, "mqtturl", g_mqtturl);
    // if (err != ESP_OK) {
    //     printf("Error setting MQTT URL to NVS!\n");
    //     return;
    // }

    //  err = nvs_set_str(nvs_handle, "mqttport", g_mqttport);
    // if (err != ESP_OK) {
    //     printf("Error setting MQTT port to NVS!\n");
    //     return;
    // }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        printf("Error committing NVS data!\n");
        return;
    }

    nvs_close(nvs_handle);
}
