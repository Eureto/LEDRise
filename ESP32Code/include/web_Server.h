#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <Arduino.h>
#include "config.h"

#define LEDRISE_PROTOCOL_VERSION 2

void handleInfo();
void handleLedBrightness();
void handleTurnOnOffLed();
void handleLedStateStatus();
void handleSetFlashing();
void handleLedOnTime();
void handleAlarmList();
void handleAlarmSet();
void handleAlarmDelete();
void handleNotFound();
void configServer();
void handleStopAlarm();

extern WebServer server;
extern bool isLedOn;

#endif
