#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include "config.h"
#include "alarm.h"

Alarm alarms[MAX_ALARMS];
DeviceSettings deviceSettings = {5, 0};

static Preferences prefs;
static time_t lastFiredAlarmTime[MAX_ALARMS]; // RAM only: prevents re-triggering after the user stops a sequence

static volatile bool sequenceRunning = false;
static volatile bool stopRequested = false;

struct SequenceParams {
  time_t start;  // when the brightness ramp begins
  time_t end;    // alarm time: LED reaches full brightness
  int ledOnMinutes;
  int flashes;
};
static SequenceParams seqParams;

// ---------------------------------------------------------------- storage

void alarmsBegin() {
  prefs.begin("ledrise", false);

  memset(alarms, 0, sizeof(alarms));
  if (prefs.getBytesLength("alarms") == sizeof(alarms)) {
    prefs.getBytes("alarms", alarms, sizeof(alarms));
  }
  // sanitize whatever came from flash
  for (int i = 0; i < MAX_ALARMS; i++) {
    Alarm &a = alarms[i];
    if (!a.used) { memset(&a, 0, sizeof(a)); continue; }
    if (a.hour > 23 || a.minute > 59 || a.preMinutes < 1 || a.preMinutes > 120 || (a.days & 0x7F) == 0) {
      memset(&a, 0, sizeof(a));
    }
  }

  deviceSettings.ledOnMinutes = prefs.getUChar("ledOnMin", 5);
  deviceSettings.flashingRepetitions = prefs.getUShort("flashes", 0);
  Serial.println("Alarms loaded from flash");
}

void alarmsSave() {
  prefs.putBytes("alarms", alarms, sizeof(alarms));
}

void settingsSave() {
  prefs.putUChar("ledOnMin", deviceSettings.ledOnMinutes);
  prefs.putUShort("flashes", deviceSettings.flashingRepetitions);
}

// ---------------------------------------------------------------- JSON

String alarmToJson(int id) {
  const Alarm &a = alarms[id];
  String s = "{\"id\":" + String(id);
  s += ",\"hour\":" + String(a.hour);
  s += ",\"minute\":" + String(a.minute);
  s += ",\"pre\":" + String(a.preMinutes);
  s += ",\"days\":" + String(a.days);
  s += ",\"repeat\":" + String(a.repeat ? "true" : "false");
  s += ",\"enabled\":" + String(a.enabled ? "true" : "false");
  s += "}";
  return s;
}

String alarmsToJson() {
  String s = "{\"alarms\":[";
  bool first = true;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (!alarms[i].used) continue;
    if (!first) s += ",";
    first = false;
    s += alarmToJson(i);
  }
  s += "]}";
  return s;
}

// ---------------------------------------------------------------- sequence

static void sleepInterruptible(uint32_t ms) {
  uint32_t until = millis() + ms;
  while (!stopRequested && (int32_t)(until - millis()) > 0) {
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

static void sequenceTask(void *) {
  Serial.println("Alarm sequence started");
  isLedOn = true;

  // Sunrise: brightness is derived from the real clock, so it always reaches 255
  // exactly at the alarm time (also across DST changes / NTP corrections).
  while (!stopRequested) {
    time_t now = time(nullptr);
    if (now >= seqParams.end) break;
    long total = (long)(seqParams.end - seqParams.start);
    long elapsed = (long)(now - seqParams.start);
    int brightness = total > 0 ? (int)((elapsed * 255L) / total) : 255;
    brightness = constrain(brightness, 0, 255);
    analogWrite(LED_PIN, brightness);
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  if (!stopRequested) {
    analogWrite(LED_PIN, 255);
    sleepInterruptible(10000);
  }

  for (int i = 0; i < seqParams.flashes && !stopRequested; i++) {
    analogWrite(LED_PIN, 0);
    sleepInterruptible(100);
    analogWrite(LED_PIN, 255);
    sleepInterruptible(100);
  }

  if (!stopRequested) {
    analogWrite(LED_PIN, 255);
    sleepInterruptible((uint32_t)seqParams.ledOnMinutes * 60000UL);
  }

  analogWrite(LED_PIN, 0);
  isLedOn = false;
  Serial.println("Alarm sequence finished");
  sequenceRunning = false;
  vTaskDelete(NULL);
}

static void fireAlarm(int id, time_t start, time_t end) {
  Alarm &a = alarms[id];
  Serial.printf("Firing alarm %d (%02d:%02d, pre %d min)\n", id, a.hour, a.minute, a.preMinutes);

  lastFiredAlarmTime[id] = end;
  if (!a.repeat) {  // one-shot: disable right away so it survives reboots as "done"
    a.enabled = 0;
    alarmsSave();
  }

  seqParams = {start, end, deviceSettings.ledOnMinutes, deviceSettings.flashingRepetitions};
  stopRequested = false;
  sequenceRunning = true;
  xTaskCreatePinnedToCore(sequenceTask, "alarmSequence", 4096, NULL, 1, NULL, tskNO_AFFINITY);
}

// Checks (at most once per second) whether any alarm's sunrise window is active.
// Works on local wall-clock time via mktime(), so DST changes are handled by the C library.
void alarmsTick() {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < 1000) return;
  lastCheck = millis();

  if (sequenceRunning) return;
  time_t now = time(nullptr);
  if (now < MIN_VALID_TIME) return;

  struct tm base;
  localtime_r(&now, &base);

  for (int i = 0; i < MAX_ALARMS; i++) {
    const Alarm &a = alarms[i];
    if (!a.used || !a.enabled) continue;

    // today and tomorrow: sunrise may begin on the evening before the alarm day
    for (int d = 0; d <= 1; d++) {
      struct tm t = base;
      t.tm_mday += d;
      t.tm_hour = a.hour;
      t.tm_min = a.minute;
      t.tm_sec = 0;
      t.tm_isdst = -1;
      time_t alarmTime = mktime(&t);  // also normalizes t.tm_wday
      if (!(a.days & (1 << t.tm_wday))) continue;

      time_t start = alarmTime - (time_t)a.preMinutes * 60;
      if (now >= start && now < alarmTime && lastFiredAlarmTime[i] != alarmTime) {
        fireAlarm(i, start, alarmTime);
        return;
      }
    }
  }
}

void alarmResetFired(int id) { lastFiredAlarmTime[id] = 0; }

bool alarmSequenceRunning() { return sequenceRunning; }

bool alarmStopSequence() {
  if (!sequenceRunning) return false;
  stopRequested = true;
  for (int i = 0; i < 100 && sequenceRunning; i++) delay(20);  // wait for the task to turn the LED off
  return true;
}
