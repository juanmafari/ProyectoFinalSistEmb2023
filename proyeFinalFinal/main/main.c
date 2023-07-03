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
#include "mqtt_client.h"
#include "softAPSTA.h"
#include "MQTTThings.h"
#include "sensor.h"
#include "esp_sntp.h"
#include "time.h"
#include "timeNTP.h"

char* TAG = "mainTest";














void app_main(void)
{
    esp_log_level_set("mainTest", ESP_LOG_INFO); 

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    softapsta(); // seteo y conexión en modo ap y espera a credenciales desde pagina web
    vTaskDelay(50000/portTICK_PERIOD_MS); //delay para esperar conexion a station, mejorar con err check
    sync_time();
    /*
    mqtt_app_start(); // inicia conexion a mqtt con dashboard de thingsboard predefinido
    

    init_sensor();
    
    
    
    

    while (1){
        

        char mensajeThi[90];
        memset(mensajeThi, 0, 90);
        uint16_t sensorValue = 100;
        //uint16_t sensorValue = get_sensor_value();
        sprintf(mensajeThi,"{\"ts\": 10000, \"values\":{\"temperature\": %d, \"humidity\": 100}}" , sensorValue);
        
        publish_telemetry(mensajeThi);
        vTaskDelay(5000/portTICK_PERIOD_MS); 
        ESP_LOGI(TAG,"RESTARTEEEEO");
        //esp_restart();
    }
    */
}
