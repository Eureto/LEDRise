#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "web_Server.h"


struct AlarmConfig {
  String alarmTime;  // Format: "HH:MM"
  int Hour;
  int Minute;    
  int preAlarmMinutes;  // Minutes before alarm to start pre-alarm
  bool isSet;           // Flag to indicate if alarm is configured
  int flashingRepetitions; // Interval for LED flashing in milliseconds
  int minutesLedON;
  bool alarmSequenceStarted;
};


extern AlarmConfig alarmConfig;


void startAlarm(void *parameter);

#endif
