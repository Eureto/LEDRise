#include "ledSignals.h"
#include "config.h"

// used to blink led in different conditions (e.g., connecting to WiFi, waiting for clock update)
void ledSignals(void *parameter)
{
  LedBlinkingParams* params = (LedBlinkingParams*)parameter;
  pinMode(Internal_LED, OUTPUT);
  while(true){
    analogWrite(Internal_LED, params->brightness);
    delay(params->blinkSpeedOn);
    analogWrite(Internal_LED, 0);
    delay(params->blinkSpeedOff);
  }
}


void ledSignals(void *parameter, int repetitions)
{
  LedBlinkingParams* params = (LedBlinkingParams*)parameter;
  pinMode(Internal_LED, OUTPUT);
  for(int i=0; i<repetitions; i++){
    analogWrite(Internal_LED, params->brightness);
    delay(params->blinkSpeedOn);
    analogWrite(Internal_LED, 0);
    delay(params->blinkSpeedOff);
  }
}