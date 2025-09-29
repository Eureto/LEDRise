#include "ledSignals.h"
#include "config.h"

// used to blink led in different conditions (e.g., connecting to WiFi, waiting for clock update)
void ledSignals(void *parameter)
{
  LedBlinkingParams* params = (LedBlinkingParams*)parameter;
  pinMode(Internal_LED, OUTPUT);
  while(true){
    digitalWrite(Internal_LED, HIGH);
    delay(params->blinkSpeedOn);
    digitalWrite(Internal_LED, LOW);
    delay(params->blinkSpeedOff);
  }
}


void ledSignals(void *parameter, int repetitions)
{
  LedBlinkingParams* params = (LedBlinkingParams*)parameter;
  pinMode(Internal_LED, OUTPUT);
  for(int i=0; i<repetitions; i++){
    digitalWrite(Internal_LED, HIGH);
    delay(params->blinkSpeedOn);
    digitalWrite(Internal_LED, LOW);
    delay(params->blinkSpeedOff);
  }
}