#ifndef LED_SIGNALS_H
#define LED_SIGNALS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct LedBlinkingParams {
  int blinkSpeedOn;
  int blinkSpeedOff;
  int brightness;
};

void ledSignals(void *parameter);
void ledSignals(void *parameter, int repetitions);

#endif
