#include "rtc.h"
#include <Arduino.h>
#include "config.h"
#include "ledSignals.h"

TaskHandle_t ledTaskHandle = NULL;

// setup RTC with NTP server to poland time and then display it 
void initializeTime() {
  // long blink short noBlink
  static LedBlinkingParams LedBlinkingParams = {1000, 100, 2}; 
  //Create task indicating connection to wifi
  xTaskCreatePinnedToCore(
    ledSignals,        // Task function
    "LedSignalsTime",  // Task name
    1024,              // Stack size
    &LedBlinkingParams,// Task parameters
    1,                 // Priority
    &ledTaskHandle,    // Task handle
    tskNO_AFFINITY     // Core ID (0 or 1)
  );


  Serial.println("Initializing time from NTP server for Poland timezone...");
  
  // Configure time with NTP server for Poland (UTC+1, DST)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Wait for time to be set
  Serial.print("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  
  Serial.println();
  Serial.println("Time synchronized successfully with Poland timezone!");
  
  if (ledTaskHandle != NULL) {
    vTaskDelete(ledTaskHandle);
    ledTaskHandle = NULL;
    analogWrite(Internal_LED, 0); // Ensure LED is turned off after time sync
  }

  printTimeRTC();
}

void printTimeRTC(){
  //print time directly from RTC
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  if (timeinfo != NULL) {
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %A", timeinfo);
    Serial.printf("RTC Time: %s\n", buffer);
    //Serial.printf("Raw timestamp: %ld\n", now);
  } else {
    Serial.println("Failed to read RTC time");
    initializeTime(); // Reinitialize time if failed
  }
  

}
