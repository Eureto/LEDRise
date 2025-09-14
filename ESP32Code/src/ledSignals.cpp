#include "ledSignals.h"
#include "config.h"

void ledSignals(void *parameter)
{
  LedParams* params = (LedParams*)parameter;
  pinMode(Internal_LED, OUTPUT);
  while(true){
    digitalWrite(Internal_LED, HIGH);
    delay(params->blinkSpeedOn);
    digitalWrite(Internal_LED, LOW);
    delay(params->blinkSpeedOff);
  }
}