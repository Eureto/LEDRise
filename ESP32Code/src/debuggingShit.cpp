#include "debuggingShit.h"
#include <WiFi.h>
#include "config.h"
#include "rtc.h"

TaskHandle_t alarmTaskHandle = NULL;

void printTimeINFINITELY(){

    xTaskCreatePinnedToCore(
    printTimeTask,   // Task function
    "TimeTracking",     // Task name
    4096,              // Stack size
    NULL,              // Task parameters
    1,                 // Priority
    NULL,              // Task handle
    0                  // Core ID (0 or 1)
  );

}


void printTimeTask(void *parameter) {
  Serial.println("Time tracking task started");
  
  while (true) {
   
    // Print current time every 10 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 10000) {
      lastPrint = millis();

      printTimeRTC();
    }
    
    // Check if we need to resync with NTP (every hour)
    static unsigned long lastSync = 0;
    if (millis() - lastSync > 3600000) { // 1 hour
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Resyncing with NTP server...");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        lastSync = millis();
      }
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay 1 second
  }
}