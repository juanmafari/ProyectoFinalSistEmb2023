#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include "MQTTThings.h"
#include "cJSON.h"
#include "nvs_flash.h"

//#define BROKER_URL "mqtt://demo.thingsboard.io"
//#define BROKER_PORT 1883
#define TOPIC "v1/devices/me/telemetry"
#define BROKER_USER "ncjxeg28pxsrauzpwx8v"
#define BROKER_PASS ""


static const char *TAG = "MQTT_EXAMPLE";

esp_mqtt_client_handle_t client;

char c_mqtturl[256]="";
char c_mqttport[20]="";


void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
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
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}



int publish_telemetry( const char* data)
{
    return esp_mqtt_client_publish(client, TOPIC, data, strlen(data), 1, 1);
}

void configMQTT(void){
    char mqttURL[100] = "mqtt://";
    strcat(mqttURL, c_mqtturl);
    printf("datoo %s \n",mqttURL);
    mqttURL[sizeof(mqttURL) - 1] = '\0';
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = (char*)mqttURL,
        .broker.address.port = strtoul(c_mqttport, NULL, 10),
        .credentials.username = BROKER_USER,
        .credentials.authentication.password = BROKER_PASS,
    }; 
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_app_start(void)
{
    
    nvs_handle_t nvs_handle;
    esp_err_t err;
    // Abrir el espacio de almacenamiento NVS
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error al abrir el espacio de almacenamiento NVS\n");
        return;
    }
    // Leer los valores de las claves
    size_t mqtturl_length = sizeof(c_mqtturl) - 1;
    err = nvs_get_str(nvs_handle, "mqtturl", c_mqtturl, &mqtturl_length);
    if (err != ESP_OK) {
        printf("Error al leer el valor de la clave 'mqtt URL' desde NVS\n");
    }

     size_t mqttport_length = sizeof(c_mqttport) - 1;
    err = nvs_get_str(nvs_handle, "mqttport", c_mqttport, &mqttport_length);
    if (err != ESP_OK) {
        printf("Error al leer el valor de la clave 'mqtt port' desde NVS\n");
    }

    printf("DATOS EN NVS \n MQTT URL: %s, MQTT PORT: %s,\n",c_mqtturl,c_mqttport );
    // Cerrar el espacio de almacenamiento NVS
    nvs_close(nvs_handle);  // El arreglo de caracteres donde se almacenará la cadena

// Utilizar sprintf() para convertir el número entero a una cadena de caracteres
    //sprintf(porto, "%d", c_mqttport);

    

    configMQTT();
    //strncpy((char*)mqtt_cfg.broker.address.uri, c_mqtturl, sizeof(mqtt_cfg.broker.address.uri));
    //strncpy((char*)mqtt_cfg.broker.address.port, c_mqttport, sizeof(mqtt_cfg.broker.address.port));

    
    
}


