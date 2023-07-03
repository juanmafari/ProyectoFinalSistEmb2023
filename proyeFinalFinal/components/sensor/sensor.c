#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define queue_value_size 10
#define BUFFER_SIZE 100

int sensor_value; 
QueueHandle_t queue_value;
int* buffer;
time_t* timestamps;
int buffer_index = 0;
int buffer_count = 0;

void sensor_task(void* pParam); //tarea que simula el sensor
void value_queue_task(void* pParam); //tarea que envia el valor a la cola y lo imprime
void logger_task(void* pParam); //tarea que recibe el valor del sensor en un buffer circular 
void initialize_buffer(); //funcion que asigna memoria al buffer y al tiempo


void init_sensor()
{
	sensor_value = 0;
	queue_value = xQueueCreate(queue_value_size, 100);
	initialize_buffer();

    xTaskCreate(sensor_task, "sensor_task", 1024*2, NULL, 9, NULL);
	xTaskCreate(value_queue_task, "sensor_value_task", 1024*2, NULL, 10, NULL);
	xTaskCreate(logger_task, "logger_task", 1024*2, NULL, 11, NULL);
}

void sensor_task(void* pParam)
{
    while (1)
	{
		sensor_value++;
		if (sensor_value > 1000)
			sensor_value = 0;
		
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

uint16_t get_sensor_value()
{
    return sensor_value;
}

//tarea que guarda en queue el valor del sensor y lo imprima

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
            //tiempoprueba = valueNtimeRX[1];
            
            buffer_index = (buffer_index + 1) % BUFFER_SIZE;
            if (buffer_count < BUFFER_SIZE)
            {
                buffer_count++;
            }
            printf("Medida almacenada - Valor: %lld, tiempo: %lld \n", valueNtimeRX[0], valueNtimeRX[1]);
        }
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
