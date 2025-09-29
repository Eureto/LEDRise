#include "web_Server.h"
#include <Arduino.h>
#include <WiFi.h>
#include "alarm.h"
WebServer server(80);

TaskHandle_t alarmTaskHandle = NULL;

void configServer(){
  server.on("/setalarm", handleSetAlarm);
  server.on("/status", handleStatus);
  server.on("/stopalarm", handleStopAlarm);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Set alarm: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/setalarm?time=HH:MM&prealarm=MINUTES");
  
}
// this function handles settings for alarm via HTTP GET request
void handleSetAlarm() {
  if (server.hasArg("time") && server.hasArg("prealarm")) {
    String time = server.arg("time");
    int preAlarm = server.arg("prealarm").toInt();
    
    // Validate time format (HH:MM)
    if (time.length() == 5 && time.charAt(2) == ':') {
      int hour = time.substring(0, 2).toInt();
      int minute = time.substring(3, 5).toInt();
      
      // Validate hour and minute ranges
      if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && preAlarm > 0 && preAlarm <= 120) {
        // Save to memory
        alarmConfig.alarmTime = time;
        alarmConfig.Hour = hour;
        alarmConfig.Minute = minute;
        alarmConfig.preAlarmMinutes = preAlarm;
        alarmConfig.isSet = true;
        
        Serial.printf("Alarm set: %s with %d minutes pre-alarm\n", time.c_str(), preAlarm);
        server.send(200, "text/plain", "OK");

        xTaskCreatePinnedToCore(
          startAlarm,        // Task function
          "alarmSequence",  // Task name
          4096,              // Stack size
          NULL,// Task parameters
          1,                 // Priority
          &alarmTaskHandle,    // Task handle
          tskNO_AFFINITY     // Core ID (0 or 1)
        );

        //startAlarm(); // Start the alarm task if not already running
      } else {
        server.send(400, "text/plain", "Invalid time or prealarm values");
      }
    } else {
      server.send(400, "text/plain", "Invalid time format. Use HH:MM");
    }
  } else {
    server.send(400, "text/plain", "Missing parameters: time and prealarm required");
  }
}

void handleStatus() {
  String response = "";
  
  if (alarmConfig.isSet) {
    response = "ALARM_SET:" + alarmConfig.alarmTime + " and preminutes: " + String(alarmConfig.preAlarmMinutes);
  } else {
    response = "NO_ALARM";
  }
  
  server.send(200, "text/plain", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void handleStopAlarm() {
  if (alarmConfig.isSet) {
    alarmConfig.isSet = false; // Reset alarm
    if (alarmTaskHandle != NULL) {
      vTaskDelete(alarmTaskHandle);
      alarmTaskHandle = NULL;
    }
    analogWrite(LED_PIN, 0); // Turn off LED if it was on
    Serial.println("Alarm stopped by user");
    server.send(200, "text/plain", "Alarm stopped");
  } else {
    server.send(400, "text/plain", "No alarm is set");
  }
}