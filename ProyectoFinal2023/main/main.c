#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"


#include <stddef.h>

#include "tbc_mqtt_helper.h"
#include "protocol_examples_common.h"

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with the config you want
   - ie #define EXAMPLE_ESP_WIFI_SSID "mywifissid" */

#define EXAMPLE_ESP_WIFI_SSID_STA      "caliope"
#define EXAMPLE_ESP_WIFI_PASS_STA      "sinlugar"
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



static const char *TAG = "EXAM_ACCESS_TOKEN_WO_SSL";

#define TELEMETYR_TEMPRATUE         	"temprature"
#define TELEMETYR_HUMIDITY          	"humidity"


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
        //ESP_LOGI(TAG_AP, "STATION " MACSTR " JOIN, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        //ESP_LOGI(TAG_AP, "STATION " MACSTR " LEAVE, AID=%d", MAC2STR(event->mac), event->aid);
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


//Don't call TBCMH API in this callback!
//Free return value by caller/(tbcmh library)!
tbcmh_value_t* tb_telemetry_on_get_temperature(void)
{
    ESP_LOGI(TAG, "Get temperature (a time-series data)");
    static float temp_array[] = {25.0, 25.5, 26.0, 26.5, 27.0, 27.5, 28.0, 27.5, 27.0, 26.5};
    static int i = 0;

    cJSON* temp = cJSON_CreateNumber(temp_array[i]);
    i++;
    i = i % (sizeof(temp_array)/sizeof(temp_array[0]));

    return temp;
}

//Don't call TBCMH API in this callback!
//Free return value by caller/(tbcmh library)!
tbcmh_value_t* tb_telemetry_on_get_humidity(void)
{
    ESP_LOGI(TAG, "Get humidity (a time-series data)");

    static int humi_array[] = {26, 27, 28, 29, 30, 31, 32, 31, 30, 29};
    static int i = 0;

    cJSON* humi = cJSON_CreateNumber(humi_array[i]);
    i++;
    i = i % (sizeof(humi_array)/sizeof(humi_array[0]));

    return humi;
}

void tb_telemetry_send(tbcmh_handle_t client)
{
    ESP_LOGI(TAG, "Send telemetry: %s, %s", TELEMETYR_TEMPRATUE, TELEMETYR_HUMIDITY);

    cJSON *object = cJSON_CreateObject(); // create json object
    cJSON_AddItemToObject(object, TELEMETYR_TEMPRATUE, tb_telemetry_on_get_temperature());
    cJSON_AddItemToObject(object, TELEMETYR_HUMIDITY, tb_telemetry_on_get_humidity());
    tbcmh_telemetry_upload_ex(client, object, 1/*qos*/, 0/*retain*/);
    cJSON_Delete(object); // delete json object
}

/*!< Callback of connected ThingsBoard MQTT */
void tb_on_connected(tbcmh_handle_t client, void *context)
{
    ESP_LOGI(TAG, "Connected to thingsboard server!");
}

/*!< Callback of disconnected ThingsBoard MQTT */
void tb_on_disconnected(tbcmh_handle_t client, void *context)
{
    ESP_LOGI(TAG, "Disconnected from thingsboard server!");
}

static void mqtt_app_start(void)
{
	//tbc_err_t err;

    ESP_LOGI(TAG, "Init tbcmh ...");
    tbcmh_handle_t client = tbcmh_init();
    if (!client) {
        ESP_LOGE(TAG, "Failure to init tbcmh!");
        return;
    }

    ESP_LOGI(TAG, "Connect tbcmh ...");
    tbc_transport_config_t transport = {
        .address = {    /*!< MQTT: broker, HTTP: server, CoAP: server */
            //bool tlsEnabled;                          /*!< Enabled TLS/SSL or DTLS */
            .schema = "mqtt",                           /*!< MQTT: mqtt/mqtts/ws/wss, HTTP: http/https, CoAP: coap/coaps */
            .host = CONFIG_TBC_TRANSPORT_ADDRESS_HOST,  /*!< MQTT/HTTP/CoAP server domain, hostname, to set ipv4 pass it as string */
            .port = CONFIG_TBC_TRANSPORT_ADDRESS_PORT,  /*!< MQTT/HTTP/CoAP server port */
            .path = NULL,                               /*!< Path in the URI*/
        },
        .credentials = {/*!< Client related credentials for authentication */
             .type = TBC_TRANSPORT_CREDENTIALS_TYPE_ACCESS_TOKEN,
                                                        /*!< ThingsBoard Client transport authentication/credentials type */
             .client_id = NULL,                         /*!< MQTT.           default client id is ``ESP32_%CHIPID%`` where %CHIPID% are last 3 bytes of MAC address in hex format */
             .username = NULL,                          /*!< MQTT/HTTP.      username */
             .password = NULL,                          /*!< MQTT/HTTP.      password */

             .token = CONFIG_TBC_TRANSPORT_ACCESS_TOKEN,/*!< MQTT/HTTP/CoAP: username/path param/path param */
                                                        /*!< At TBC_TRANSPORT_CREDENTIALS_TYPE_X509 it's a client public key. DON'T USE IT! */
        },
        .verification = { /*!< Security verification of the broker/server */
             //bool      use_global_ca_store;       /*!< Use a global ca_store, look esp-tls documentation for details. */
             //esp_err_t (*crt_bundle_attach)(void *conf); 
                                                    /*!< Pointer to ESP x509 Certificate Bundle attach function for the usage of certificate bundles. */
             .cert_pem = NULL,                      /*!< Pointer to certificate data in PEM or DER format for server verify (with SSL), default is NULL, not required to verify the server. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in cert_len. */
             .cert_len = 0,                         /*!< Length of the buffer pointed to by cert_pem. May be 0 for null-terminated pem */
             //const struct psk_key_hint *psk_hint_key;
                                                    /*!< Pointer to PSK struct defined in esp_tls.h to enable PSK
                                                      authentication (as alternative to certificate verification).
                                                      PSK is enabled only if there are no other ways to
                                                      verify broker.*/
             .skip_cert_common_name_check = false,  /*!< Skip any validation of server certificate CN field, this reduces the security of TLS and makes the mqtt client susceptible to MITM attacks  */
             //const char **alpn_protos;            /*!< NULL-terminated list of supported application protocols to be used for ALPN */
        },
        .authentication = { /*!< Client authentication for mutual authentication using TLS */
             .client_cert_pem = NULL,           /*!< Pointer to certificate data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_key_pem` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_cert_len. */
             .client_cert_len = 0,              /*!< Length of the buffer pointed to by client_cert_pem. May be 0 for null-terminated pem */
             .client_key_pem = NULL,            /*!< Pointer to private key data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_cert_pem` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_key_len */
             .client_key_len = 0,               /*!< Length of the buffer pointed to by client_key_pem. May be 0 for null-terminated pem */
             .client_key_password = NULL,       /*!< Client key decryption password string */
             .client_key_password_len = 0,      /*!< String length of the password pointed to by client_key_password */
             //bool      use_secure_element;    /*!< Enable secure element, available in ESP32-ROOM-32SE, for SSL connection */
             //void     *ds_data;               /*!< Carrier of handle for digital signature parameters, digital signature peripheral is available in some Espressif devices. */

        },

        .log_rxtx_package = true                /*!< print Rx/Tx MQTT package */
    };
    
    bool result = tbcmh_connect(client, &transport,
                        NULL, tb_on_connected, tb_on_disconnected);
    if (!result) {
        ESP_LOGE(TAG, "failure to connect to tbcmh!");
        goto exit_destroy;
    }


    ESP_LOGI(TAG, "connect tbcmh ...");
    int i = 0;
    while (i<20) {
        if (tbcmh_has_events(client)) {
            tbcmh_run(client);
        }

        i++;
        if (tbcmh_is_connected(client)) {
            if (i%5 == 0){
                tb_telemetry_send(client);
            }
        } else {
            ESP_LOGI(TAG, "Still NOT connected to server!");
        }
        sleep(1);
    }


    ESP_LOGI(TAG, "Disconnect tbcmh ...");
    tbcmh_disconnect(client);

exit_destroy:
    ESP_LOGI(TAG, "Destroy tbcmh ...");
    tbcmh_destroy(client);
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
