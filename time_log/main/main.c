#include <stdio.h>
#include "esp_sntp.h"
#include "esp_log.h"
#include "time.h"

//Declarations
static const char *TAG = "main";
const char* ntpServer = "pool.ntp.org";

//Functions

void configure(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL); 
    sntp_setservername(0, ntpServer); // set server
    sntp_init (); //initializes and updates sys time to ntp time (syncs)
}

void sync_time(void){

    configure();

    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        ESP_LOGI(TAG, "Synchronizing...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Time is synchronized!");
}


//Main

void app_main(void)
{
    sync_time();

    time_t now = time(NULL); //retrieve current system time, returns seconds since epoch
    struct tm timeinfo; //struct to where time info will be saved
    char buffer[26]; //array to where the formatted time string will be saved

    gmtime_r(&now, &timeinfo); //convert seconds to struct tm type; utc time
    timeinfo.tm_hour -= 3; //local timezone gmt -3
    printf("Local time: %s", asctime_r(&timeinfo, buffer));
}


/*va a quedar en syncing... eternamente porque este código no incluye conexión a internet
para que pueda acceder al servidor*/