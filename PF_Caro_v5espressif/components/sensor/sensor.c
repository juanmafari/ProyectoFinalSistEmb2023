#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"



int sensor_value; 



void sensor_task(void* pParam); //tarea que simula el sensor


void init_sensor()
{
	sensor_value = 0;
	xTaskCreate(sensor_task, "sensor_task", 1024*2, NULL, 9, NULL);
	
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





