#include "web_Server.h"
#include <Arduino.h>
#include <WiFi.h>
#include "alarm.h"
WebServer server(80);

TaskHandle_t alarmTaskHandle = NULL;

void configServer(){
  server.on("/setalarm", handleSetAlarm);
  server.on("/alarmStatus", handleAlarmStatus);
  server.on("/stopalarm", handleStopAlarm);
  server.on("/setflashing", handleSetFlashing);
  server.on("/ledonminutes", handleLEDOnTime);
  server.on("/turnOnOffLed", handleTurnOnOffLed);
  server.on("/ledStateStatus", handleLedStateStatus);
  server.on("/setBrightness", handleLedBrightness);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Set alarm: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/setalarm?time=HH:MM&prealarm=MINUTES");
  
}


void handleLedBrightness() {
  if (server.hasArg("value")) {
    int brightness = server.arg("value").toInt();
    if (brightness >= 0 && brightness <= 255) {
      analogWrite(LED_PIN, brightness);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid brightness value. Must be between 0 and 255.");
    }
  } else {
    server.send(400, "text/plain", "Missing parameter: value");
  }
}

void handleTurnOnOffLed() {
  
  if(digitalRead(LED_PIN) == HIGH) {
    analogWrite(LED_PIN, 0); // Turn off LED
    server.send(200, "text/plain", "LED turned off");
    return;
  }else{
    // If LED is off, turn it on
    analogWrite(LED_PIN, 255); // Turn on LED at full brightness
    server.send(200, "text/plain", "LED turned on");
    return;
  }

}

void handleLedStateStatus() {

  String ledState = digitalRead(LED_PIN) == HIGH ? "ON" : "OFF";
  server.send(200, "text/plain", ledState);
}

void handleLEDOnTime() {
  if (server.hasArg("minutes")) {
    int minutes = server.arg("minutes").toInt();
    if (minutes >= 0 && minutes <= 60) {
      alarmConfig.minutesLedON = minutes;
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid minutes value. Must be between 0 and 60.");
    }
  } else {
    server.send(400, "text/plain", "Missing parameter: minutes");
  }
}

// this function handles settings for flashing repetitions after preAlarm sequence
// Example: /setflashing?repetitions=50
void handleSetFlashing() {
  if (server.hasArg("repetitions")) {
    int repetitions = server.arg("repetitions").toInt();
    
    // Validate repetitions range
    if (repetitions >= 0 && repetitions <= 10000) {
      // Save to memory
      alarmConfig.flashingRepetitions = repetitions;
      
      Serial.printf("Flashing repetitions set to: %d ms\n", repetitions);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid repetitions value. Must be between 0 and 10000 steps");
    }
  } else {
    server.send(400, "text/plain", "Missing parameter: repetitions");
  }
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

void handleAlarmStatus() {
  String response = "";
  

  if (alarmConfig.isSet) {
    response = "ALARM_SET:" + alarmConfig.alarmTime + " and preminutes: " + String(alarmConfig.preAlarmMinutes);
    
  } else {
    response = "NO_ALARM ";

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