#include "web_Server.h"
#include <Arduino.h>
#include <WiFi.h>
#include "alarm.h"
#include <time.h>
#include <ElegantOTA.h>
#include "config.h"

bool isLedOn = false;

WebServer server(80);

void configServer(){
  server.on("/info", handleInfo);              // discovery + device state
  server.on("/alarms", handleAlarmList);       // list of alarms (JSON)
  server.on("/alarms/set", handleAlarmSet);    // create / update alarm
  server.on("/alarms/delete", handleAlarmDelete);
  server.on("/stopalarm", handleStopAlarm);    // abort the running sunrise sequence
  server.on("/setflashing", handleSetFlashing);
  server.on("/ledonminutes", handleLedOnTime);
  server.on("/turnOnOffLed", handleTurnOnOffLed);
  server.on("/ledStateStatus", handleLedStateStatus);
  server.on("/setBrightness", handleLedBrightness);
  server.onNotFound(handleNotFound);

  ElegantOTA.begin(&server);    // Start ElegantOTA, must be before server.begin()
  ElegantOTA.setAuth(OTA_USERNAME, OTA_PASSWORD); // must type ipaddress/update to access ota page
  server.begin();
  Serial.print("HTTP server started on http://");
  Serial.println(WiFi.localIP());
}

// Used by the mobile app to find the device in the local network.
void handleInfo() {
  time_t now = time(nullptr);
  struct tm tmNow;
  localtime_r(&now, &tmNow);
  char timeBuf[32];
  strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmNow);

  String json = "{\"device\":\"LedRise\",\"version\":" + String(LEDRISE_PROTOCOL_VERSION);
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"mac\":\"" + WiFi.macAddress() + "\"";
  json += ",\"time\":\"" + String(timeBuf) + "\"";
  json += ",\"timeValid\":" + String(now >= MIN_VALID_TIME ? "true" : "false");
  json += ",\"ledOn\":" + String(isLedOn ? "true" : "false");
  json += ",\"sequenceRunning\":" + String(alarmSequenceRunning() ? "true" : "false");
  json += ",\"ledOnMinutes\":" + String(deviceSettings.ledOnMinutes);
  json += ",\"flashingRepetitions\":" + String(deviceSettings.flashingRepetitions);
  json += ",\"maxAlarms\":" + String(MAX_ALARMS);
  json += "}";
  server.send(200, "application/json", json);
}

void handleLedBrightness() {
  if(alarmSequenceRunning()) {
    server.send(400, "text/plain", "Cannot change brightness while alarm is active");
    return;
  }
  if (server.hasArg("value")) {
    int brightness = server.arg("value").toInt();
    if (brightness >= 0 && brightness <= 255) {
      isLedOn = brightness > 0 ? true : false;
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
  if(alarmSequenceRunning()) {
    server.send(400, "text/plain", "Cannot toggle LED while alarm is active");
    return;
  }
  if(isLedOn) {
    analogWrite(LED_PIN, 0); // Turn off LED
    isLedOn = false;
    server.send(200, "text/plain", "LED turned off");
    return;
  }else{
    // If LED is off, turn it on
    analogWrite(LED_PIN, 255); // Turn on LED at full brightness
    isLedOn = true;
    server.send(200, "text/plain", "LED turned on");
    return;
  }

}

void handleLedStateStatus() {

  String ledState = isLedOn == true ? "ON" : "OFF";
  server.send(200, "text/plain", ledState);
}

void handleLedOnTime() {
  if (server.hasArg("minutes")) {
    int minutes = server.arg("minutes").toInt();
    if (minutes >= 0 && minutes <= 60) {
      deviceSettings.ledOnMinutes = minutes;
      settingsSave();
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
      deviceSettings.flashingRepetitions = repetitions;
      settingsSave();
      
      Serial.printf("Flashing repetitions set to: %d ms\n", repetitions);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid repetitions value. Must be between 0 and 10000 steps");
    }
  } else {
    server.send(400, "text/plain", "Missing parameter: repetitions");
  }
}

static bool parseTime(const String &t, int &hour, int &minute) {
  if (t.length() != 5 || t.charAt(2) != ':') return false;
  hour = t.substring(0, 2).toInt();
  minute = t.substring(3, 5).toInt();
  return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

void handleAlarmList() {
  server.send(200, "application/json", alarmsToJson());
}

// Create or update an alarm.
// /alarms/set?time=HH:MM&prealarm=15&days=62&repeat=1&enabled=1[&id=N]
//  - no id -> new alarm in the first free slot
//  - days: bitmask, bit0 = Sunday ... bit6 = Saturday
//  - repeat=0 -> fires once on the next matching day, then disables itself
void handleAlarmSet() {
  if (!server.hasArg("time") || !server.hasArg("prealarm") || !server.hasArg("days")) {
    server.send(400, "text/plain", "Missing parameters: time, prealarm and days required");
    return;
  }
  int hour, minute;
  if (!parseTime(server.arg("time"), hour, minute)) {
    server.send(400, "text/plain", "Invalid time format. Use HH:MM");
    return;
  }
  int pre = server.arg("prealarm").toInt();
  int days = server.arg("days").toInt();
  if (pre < 1 || pre > 120) {
    server.send(400, "text/plain", "Invalid prealarm. Must be 1-120 minutes");
    return;
  }
  if (days < 1 || days > 127) {
    server.send(400, "text/plain", "Invalid days mask. Must be 1-127");
    return;
  }

  int id = -1;
  if (server.hasArg("id")) {
    id = server.arg("id").toInt();
    if (id < 0 || id >= MAX_ALARMS || !alarms[id].used) {
      server.send(404, "text/plain", "Alarm not found");
      return;
    }
  } else {
    for (int i = 0; i < MAX_ALARMS; i++) {
      if (!alarms[i].used) { id = i; break; }
    }
    if (id < 0) {
      server.send(400, "text/plain", "Alarm limit reached");
      return;
    }
  }

  Alarm &a = alarms[id];
  a.used = 1;
  a.hour = hour;
  a.minute = minute;
  a.preMinutes = pre;
  a.days = days;
  a.repeat = server.hasArg("repeat") ? (server.arg("repeat").toInt() != 0) : 1;
  a.enabled = server.hasArg("enabled") ? (server.arg("enabled").toInt() != 0) : 1;
  alarmResetFired(id);
  alarmsSave();

  Serial.printf("Alarm %d saved: %02d:%02d pre=%d days=%d repeat=%d enabled=%d\n",
                id, hour, minute, pre, days, a.repeat, a.enabled);
  server.send(200, "application/json", alarmToJson(id));
}

void handleAlarmDelete() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain", "Missing parameter: id");
    return;
  }
  int id = server.arg("id").toInt();
  if (id < 0 || id >= MAX_ALARMS || !alarms[id].used) {
    server.send(404, "text/plain", "Alarm not found");
    return;
  }
  memset(&alarms[id], 0, sizeof(Alarm));
  alarmResetFired(id);
  alarmsSave();
  server.send(200, "text/plain", "OK");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// Stops the currently running sunrise/alarm sequence (alarm configuration is kept).
void handleStopAlarm() {
  if (alarmStopSequence()) {
    isLedOn = false;
    Serial.println("Alarm sequence stopped by user");
    server.send(200, "text/plain", "Alarm stopped");
  } else {
    server.send(400, "text/plain", "No alarm is running");
  }
}
