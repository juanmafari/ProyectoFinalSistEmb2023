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
	queue_value = xQueueCreate(queue_value_size, sizeof(int));
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
    while (1)
    {
        value = get_sensor_value();
        if (xQueueSend(queue_value, &value, portMAX_DELAY) != pdPASS)
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
    char timeString[50];
    
    while (1)
    {
        if (xQueueReceive(queue_value, &value, portMAX_DELAY) == pdTRUE)
        {
            timestamp = time(NULL);
            timeinfo = localtime(&timestamp);
            
            strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);
            
            buffer[buffer_index] = value;
            timestamps[buffer_index] = timestamp;
            
            buffer_index = (buffer_index + 1) % BUFFER_SIZE;
            if (buffer_count < BUFFER_SIZE)
            {
                buffer_count++;
            }
            printf("Medida almacenada - Valor: %d, Fecha y Hora: %s\n", value, timeString);
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
