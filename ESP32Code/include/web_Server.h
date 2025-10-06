#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <Arduino.h>
#include "config.h"

void handleLedBrightness();
void handleTurnOnLed();
void handleTurnOffLed();
void handleSetAlarm();
void handleSetFlashing();
void handleLEDOnTime();
void handleStatus();
void handleNotFound();
void configServer();
void handleStopAlarm();

extern WebServer server;

#endif
