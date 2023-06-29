#include <stdio.h>
#include "sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define queue_value_size 10

int sensor_value; 
QueueHandle_t queue_value;
void sensor_task(void* pParam);
void value_queue_task(void* pParam);


void init_sensor()
{
	sensor_value = 0;
	queue_value = xQueueCreate(queue_value_size, sizeof(int));
	
    xTaskCreate(sensor_task, "sensor_task", 1024*2, NULL, 9, NULL);
	xTaskCreate(value_queue_task, "sensor_value_task", 1024*2, NULL, 10, NULL);
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
        
        printf("Valor del sensor: %d\n", value);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

