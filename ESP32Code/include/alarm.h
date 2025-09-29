#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>


struct AlarmConfig {
  String alarmTime;  // Format: "HH:MM"
  int Hour;
  int Minute;    
  int preAlarmMinutes;  // Minutes before alarm to start pre-alarm
  bool isSet;           // Flag to indicate if alarm is configured
};


extern AlarmConfig alarmConfig;


void startAlarm(void *parameter);

#endif
