#include "web_Server.h"
#include <Arduino.h>
#include <WiFi.h>
WebServer server(80);

AlarmConfig alarmConfig = {"", 0, 0, 0, false};

void configServer(){
  server.on("/setalarm", handleSetAlarm);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Set alarm: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/setalarm?time=HH:MM&prealarm=MINUTES");
  
}

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
        startAlarm(); // Start the alarm task if not already running
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
    response = "ALARM_SET:" + alarmConfig.alarmTime + ":" + String(alarmConfig.preAlarmMinutes);
  } else {
    response = "NO_ALARM";
  }
  
  server.send(200, "text/plain", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}


void startAlarm()
{
  time_t tempTime = time(nullptr);
  //calculate seconds to xx:00 minutes 
  int secondsToNextMinute = 60 - (localtime(&tempTime)->tm_sec);
  Serial.printf("Seconds to next full minute: %d\n", secondsToNextMinute);
  delay((secondsToNextMinute * 1000)-700); // Delay until next full minute
  tempTime = time(nullptr); // Update tempTime after delay

  //Calculate steps for led dimming up 
  int steps = 255 / alarmConfig.preAlarmMinutes;
  Serial.printf("Steps for dimming: %d\n", steps);

  //Calculate time to start pre-alarm - minutes after midnight
  int startTime = alarmConfig.Hour * 60 + alarmConfig.Minute - alarmConfig.preAlarmMinutes;
  Serial.printf("Start time for alarm: %d\n", startTime);

  int currentTimeInMinutes = (localtime(&tempTime)->tm_hour) * 60 + (localtime(&tempTime)->tm_min);
  Serial.printf("Current time in minutes: %d\n", currentTimeInMinutes);
  
  int toAlarmInMinutes{0};
  //calculate time to allarm in minutes 
  if(currentTimeInMinutes >= startTime) {
    toAlarmInMinutes = 24*60 - currentTimeInMinutes + startTime;
  } else {
    toAlarmInMinutes = startTime - currentTimeInMinutes;
  }
    Serial.println("toAlarmInMinutes value:" + String(toAlarmInMinutes));
    Serial.printf("Alarm should start in %d minutes\n so it is in %d hours and %d minutes \n", toAlarmInMinutes, toAlarmInMinutes / 60, toAlarmInMinutes % 60);
    delay(toAlarmInMinutes * 60000); // Delay until alarm time
    Serial.println("Starting pre-alarm sequence...");
    // Pre-alarm sequence: gradually increase LED brightness
    for (int brightness = steps; brightness < 255; brightness += steps) {
      analogWrite(LED_PIN, brightness);
      Serial.printf("LED brightness: %d\n", brightness);
      delay(60000); // Wait 1 minute between brightness increases
    }
    Serial.println("Pre-alarm sequence complete. LED at full brightness.");
    analogWrite(LED_PIN, 255); // Ensure LED is fully on
    delay(10000);
    for(int i=0; i<10; i++) {
      analogWrite(LED_PIN, 0); // Turn off LED
      delay(500);
      analogWrite(LED_PIN, 255); // Turn on LED
      delay(500);
    }
    analogWrite(LED_PIN, 255);
    delay(5 * 60 * 1000); // Keep LED on for 5 minutes
    analogWrite(LED_PIN, 0); // Turn off LED after alarm
    Serial.println("Alarm sequence complete. LED turned off.");
    alarmConfig.isSet = false; // Reset alarm 
}