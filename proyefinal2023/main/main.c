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
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
//#include "mdns.h"
#include "mqtt_client.h"
#include "softAPSTA.h"
#include "MQTTThings.h"
#include "sensor.h"


char* TAG = "mainTest";


void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    softapsta(); // seteo y conexión en modo ap+sta hibrido
    vTaskDelay(10000/portTICK_PERIOD_MS); //delay para esperar conexion a station, mejorar con err check
    mqtt_app_start(); // inicia conexion a mqtt con dashboard de thingsboard predefinido

    init_sensor();
    

    while (1){
        esp_log_level_set("mainTest", ESP_LOG_INFO); 
        char mensajeThi[50];
        memset(mensajeThi, 0, 50);
        uint16_t sensorValue = get_sensor_value();
        ESP_LOGI("mainTest", "%d %s",sensorValue,mensajeThi);
        sprintf(mensajeThi,"{\"temperature\": %d, \"humidity\": 300}",sensorValue);
        publish_telemetry(mensajeThi);
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}
