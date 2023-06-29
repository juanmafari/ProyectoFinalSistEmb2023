#include <stdio.h>
#include "sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int sensor_value; 
void sensor_task(void* pParam);


void init_sensor()
{
	sensor_value = 0;
    xTaskCreate(sensor_task, "sensor_task", 1000, NULL, 10, NULL);
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