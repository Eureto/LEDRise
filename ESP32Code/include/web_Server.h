#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <Arduino.h>
#include "config.h"

void handleLedBrightness();
void handleTurnOnOffLed();
void handleLedStateStatus();
void handleSetAlarm();
void handleSetFlashing();
void handleLedOnTime();
void handleAlarmStatus();
void handleNotFound();
void configServer();
void handleStopAlarm();

extern WebServer server;
extern TaskHandle_t alarmTaskHandle;

#endif
