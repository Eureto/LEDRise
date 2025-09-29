#include "wifiConnection.h"

extern TaskHandle_t ledTaskHandle;  // Declaration only

void connectToWiFi() {
  static LedBlinkingParams LedBlinkingParams = {200, 800}; // short blink long noBlink
  //Create task indicating connection to wifi
  xTaskCreatePinnedToCore(
    ledSignals,   // Task function
    "LedSignalsWiFi",     // Task name
    1024,              // Stack size
    &LedBlinkingParams,        // Task parameters
    1,                 // Priority
    &ledTaskHandle,    // Task handle
    tskNO_AFFINITY     // Core ID (0 or 1)
  );


  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  
  IPAddress local_IP(ESP32_IP);
  IPAddress gateway(GATEWAY_IP);
  IPAddress subnet(SUBNET_MASK);
  IPAddress primaryDNS(PRIMARY_DNS);
  IPAddress secondaryDNS(SECONDARY_DNS);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Static IP configuration failed!");
  }

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected successfully!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  if (ledTaskHandle != NULL) {
    vTaskDelete(ledTaskHandle);
    ledTaskHandle = NULL;
  }
  
}