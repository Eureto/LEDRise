#include <Arduino.h>
#include "config.h"
#include "web_Server.h"
#include "rtc.h"
#include "wifiConnection.h"
#include "ledSignals.h"
#include "debuggingShit.h"

bool DEBUG = true;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, world!");
  
  // Pin Setup
  pinMode(Internal_LED, OUTPUT);
  
  connectToWiFi();
  initializeTime();

  configServer();

  digitalWrite(Internal_LED, LOW); // Turn off internal LED after setup

  if(DEBUG)
  {
    printTimeINFINITELY();
  }

}

void loop() {
server.handleClient();  // Handle incoming requests
  delay(10);
}


