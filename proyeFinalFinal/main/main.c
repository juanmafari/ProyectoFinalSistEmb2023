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

#define queue_value_size 10
#define BUFFER_SIZE 100
int* buffer;
time_t* timestamps;
int buffer_index = 0;
int buffer_count = 0;
int publish_index = 0;
QueueHandle_t queue_value;

void value_queue_task(void* pParam); //tarea que envia el valor a la cola y lo imprime
void logger_task(void* pParam); //tarea que recibe el valor del sensor en un buffer circular 
void initialize_buffer(); //funcion que asigna memoria al buffer y al tiempo

void value_queue_task(void* pParam)
{
    int value;
    int actualTime;
    long long valueNtime[2];
    time_t now;
    while (1)
    {
        valueNtime[0] = get_sensor_value();
        now = time(NULL); //retrieve current system time, returns seconds since epoch
        now = now*1000;
        valueNtime[1] = now;
        if (xQueueSend(queue_value, &valueNtime, portMAX_DELAY) != pdPASS)
        {
            printf("Error al enviar el valor del sensor a la cola.\n");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void logger_task(void* pParam)
{
    int value;
    time_t timestamp;
    struct tm* timeinfo;
    int tiempoprueba;
    long long valueNtimeRX[2];
    
    
    while (1)
    {
        if (xQueueReceive(queue_value, &valueNtimeRX, portMAX_DELAY) == pdTRUE)
        {
            
            
            buffer[buffer_index] = valueNtimeRX[0];
            timestamps[buffer_index] = valueNtimeRX[1];
            
            
            printf("Medida almacenada - Valor: %lld, tiempo: %lld \n", valueNtimeRX[0], valueNtimeRX[1]);
            
            buffer_index = (buffer_index + 1) % BUFFER_SIZE;
            if (buffer_count < BUFFER_SIZE)
            {
                buffer_count++;
                printf("count %d",buffer_count);
            }
        }
    }
}

void publish_task(void* pParam)
{
    char mensajeThi[90];
    char last_index[50];
    while (1)
    {
        memset(mensajeThi, 0, 90);
        sprintf(mensajeThi,"{\"ts\": %lld, \"values\":{\"temperature\": %d}}" , timestamps[publish_index],buffer[publish_index]);
        if (buffer_index != last_index){
            if (publish_telemetry(mensajeThi) != -1 ){
            
            printf("Medida de buffer %d recibida por broker \n", publish_index);
            
                
            } else {
                printf("Medida de buffer %d no recibida por broker, esperando a condiciones \n", buffer_index);
                while(publish_telemetry(mensajeThi) == -1){
                    
                }
            }
            if (publish_index <= buffer_index)
            {
                //last_index = publish_index;
                publish_index = (publish_index + 1) % BUFFER_SIZE;
                
            }
        }
        
        
        
        

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void initialize_buffer()
{
    buffer = (int*)malloc(BUFFER_SIZE * sizeof(int));
    timestamps = (time_t*)malloc(BUFFER_SIZE * sizeof(time_t));
    if (buffer == NULL || timestamps == NULL)
    {
        printf("Error en la asignación de memoria para el buffer.\n");
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}












void app_main(void)
{
    esp_log_level_set("mainTest", ESP_LOG_INFO); 

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    int staConnect = softapsta(); // seteo y conexión en modo ap y espera a credenciales desde pagina web
     //delay para esperar conexion a station, mejorar con err check

    if (staConnect == 1)
    {
        printf("conexión exitosa, puede seguir \n");
        vTaskDelay(3000/portTICK_PERIOD_MS);
    }
    

    sync_time();
    init_sensor();
    mqtt_app_start(); // inicia conexion a mqtt con dashboard de thingsboard predefinido
    
    queue_value = xQueueCreate(queue_value_size, 100);
	initialize_buffer();
    xTaskCreate(value_queue_task, "sensor_value_task", 1024, NULL, 10, NULL);
	xTaskCreate(logger_task, "logger_task", 1024*2, NULL, 11, NULL);
    
    
    
    

    while (1){
        

        
        vTaskDelay(5000/portTICK_PERIOD_MS); 
        ESP_LOGI(TAG,"RESTARTEEEEO");
        //esp_restart();
    }
    
}
