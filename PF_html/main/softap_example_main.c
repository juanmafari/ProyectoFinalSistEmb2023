
/*  WiFi softAP Example
    This example code is in the Public Domain (or CC0 licensed, at your option.)
    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <esp_http_server.h>

/* The examples use WiFi configuration that you can set via project configuration menu.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "kaluga_wifi"
#define EXAMPLE_ESP_WIFI_PASS      "kalugaucu1"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static const char *TAG = "wifi softAP";

static char usuario[32] = "";
static char password[32] = "";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = 2,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

// Función archivo HTML
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

        char *ssid_pos = strstr(buf, "ssid=");
        char *password_pos = strstr(buf, "password=");

        if (ssid_pos && password_pos) {
            strncpy(usuario, ssid_pos + 5, sizeof(usuario) - 1);
            strncpy(password, password_pos + 9, sizeof(password) - 1);
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

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    start_webserver();

    // Verificar las condiciones
    
        if (strcmp(usuario, "kaluga") == 0 && strcmp(password, "kaluga1") == 0) {
            // aca iria la funcion de inicializacion de station 
            // por ahora le puse este mensaje nomas
            printf("Conexión exitosa\n");
        } else {
            // Las condiciones no se cumplen
            printf("Error de conexión: Usuario o contraseña incorrectos\n");
        }

    
}
