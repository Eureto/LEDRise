#ifndef LED_SIGNALS_H
#define LED_SIGNALS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct LedParams {
  int blinkSpeedOn;
  int blinkSpeedOff;
};

void ledSignals(void *parameter);

#endif
