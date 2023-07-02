#include <stdio.h>
#include "timeNTP.h"
#include "esp_sntp.h"
#include "time.h"
#include "esp_log.h"

static char* TAG = "timeNTP";

const char* ntpServer = "pool.ntp.org"; //declaracion server

void configureSNTP(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL); 
    sntp_setservername(0, ntpServer); // set server
    sntp_init (); //initializes and updates sys time to ntp time (syncs)
}

void sync_time(void){

    configureSNTP();

    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        ESP_LOGI(TAG, "Synchronizing...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Time is synchronized!");
}