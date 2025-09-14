#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <Arduino.h>
#include "config.h"

void handleSetAlarm();
void handleStatus();
void handleNotFound();
void configServer();
void startAlarm();

extern WebServer server;

struct AlarmConfig {
  String alarmTime;  // Format: "HH:MM"
  int Hour;
  int Minute;    
  int preAlarmMinutes;  // Minutes before alarm to start pre-alarm
  bool isSet;           // Flag to indicate if alarm is configured
};

extern AlarmConfig alarmConfig;

#endif
