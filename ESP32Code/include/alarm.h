#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>

#define MAX_ALARMS 20
#define MIN_VALID_TIME 1700000000  // anything below means the clock was not synced yet

// One alarm slot. The slot index is the alarm id.
struct Alarm {
  uint8_t used;        // slot is occupied
  uint8_t hour;        // 0-23   (time the LED reaches full brightness, local time)
  uint8_t minute;      // 0-59
  uint8_t preMinutes;  // 1-120  (length of the "sunrise" before the alarm time)
  uint8_t days;        // bitmask, bit i = weekday i (0 = Sunday ... 6 = Saturday)
  uint8_t repeat;      // 1 = every week, 0 = fire once (next matching day) and then disable
  uint8_t enabled;
};

struct DeviceSettings {
  uint8_t ledOnMinutes;         // how long LED stays on after the alarm
  uint16_t flashingRepetitions; // flashes after the sunrise
};

extern Alarm alarms[MAX_ALARMS];
extern DeviceSettings deviceSettings;
extern bool isLedOn;  // defined in web_Server.cpp

void alarmsBegin();                 // load alarms + settings from flash (NVS)
void alarmsSave();                  // persist alarms
void settingsSave();                // persist device settings
void alarmsTick();                  // call often from loop(), starts sequences when due
void alarmResetFired(int id);       // call after editing/deleting an alarm
bool alarmSequenceRunning();
bool alarmStopSequence();           // returns true if something was running
String alarmsToJson();
String alarmToJson(int id);

#endif
