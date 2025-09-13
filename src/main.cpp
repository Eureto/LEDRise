#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WebServer.h>
#include "config.h"

void connectToWiFi();
void initializeTime();
void printTime();
void timeTrackingTask(void *parameter);
void ledSignals(void *parameter);
void handleSetAlarm();
void handleStatus();
void handleNotFound();
void startAlarmTask();
void startAlarm();
void printTimeRTC();

TaskHandle_t alarmTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;
WebServer server(80);

#define LED_PIN 33
#define Internal_LED 26


// NTP server configuration for Poland
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 1 * 3600;  // Poland is UTC+1
const int daylightOffset_sec = 3600;  // DST offset (summer time)

// Time tracking variables
volatile time_t currentTime;

struct LedParams {
  int blinkSpeedOn;
  int blinkSpeedOff;
};

struct AlarmConfig {
  String alarmTime;  // Format: "HH:MM"
  int Hour;
  int Minute;    
  int preAlarmMinutes;  // Minutes before alarm to start pre-alarm
  bool isSet;           // Flag to indicate if alarm is configured
};
AlarmConfig alarmConfig = {"", 0, false};

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, world!");
  
  // Pin Setup
  pinMode(Internal_LED, OUTPUT);
  
  connectToWiFi();
  initializeTime();

  server.on("/setalarm", handleSetAlarm);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Set alarm: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/setalarm?time=HH:MM&prealarm=MINUTES");
  


  xTaskCreatePinnedToCore(
    timeTrackingTask,   // Task function
    "TimeTracking",     // Task name
    4096,              // Stack size
    NULL,              // Task parameters
    1,                 // Priority
    NULL,              // Task handle
    0                  // Core ID (0 or 1)
  );

  digitalWrite(Internal_LED, LOW); // Turn off internal LED after setup
}

void loop() {
server.handleClient();  // Handle incoming requests
  delay(10);
  printTimeRTC();
}

void connectToWiFi() {
  static LedParams ledParams = {200, 800}; // short blink long noBlink
  //Create task indicating connection to wifi
  xTaskCreatePinnedToCore(
    ledSignals,   // Task function
    "LedSignalsWiFi",     // Task name
    1024,              // Stack size
    &ledParams,        // Task parameters
    1,                 // Priority
    &ledTaskHandle,    // Task handle
    tskNO_AFFINITY     // Core ID (0 or 1)
  );


  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  
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

void initializeTime() {
  // long blink short noBlink
  static LedParams ledParams = {1000, 100}; 
  //Create task indicating connection to wifi
  xTaskCreatePinnedToCore(
    ledSignals,   // Task function
    "LedSignalsTime",     // Task name
    1024,              // Stack size
    &ledParams,        // Task parameters
    1,                 // Priority
    &ledTaskHandle,    // Task handle
    tskNO_AFFINITY     // Core ID (0 or 1)
  );


  Serial.println("Initializing time from NTP server for Poland timezone...");
  
  // Configure time with NTP server for Poland (UTC+1, DST)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Wait for time to be set
  Serial.print("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  
  Serial.println();
  Serial.println("Time synchronized successfully with Poland timezone!");
  currentTime = now;
  
  if (ledTaskHandle != NULL) {
    vTaskDelete(ledTaskHandle);
    ledTaskHandle = NULL;
  }

  printTime();
}

void printTimeRTC(){
  //print time directly from RTC
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  if (timeinfo != NULL) {
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %A", timeinfo);
    Serial.printf("RTC Time: %s\n", buffer);
    Serial.printf("Raw timestamp: %ld\n", now);
  } else {
    Serial.println("Failed to read RTC time");
    initializeTime(); // Reinitialize time if failed
  }
  

}

void printTime() {
  // print value of currentTime variable
  time_t tempTime = currentTime;
  struct tm timeinfo;
  if (localtime_r(&tempTime, &timeinfo) == NULL) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.print("Time: ");
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%A, %d %B %Y %H:%M:%S", &timeinfo);
  Serial.println(buffer);
}

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

void timeTrackingTask(void *parameter) {
  Serial.println("Time tracking task started");
  
  while (true) {
    // Update current time in RAM
    currentTime += 1; // Increment by 1 second
    // currentTime = time(nullptr); --- IGNORE ---  
    
    // Print current time every 10 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 10000) {
      lastPrint = millis();

      printTime();
    }
    
    // Check if we need to resync with NTP (every hour)
    static unsigned long lastSync = 0;
    if (millis() - lastSync > 3600000) { // 1 hour
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Resyncing with NTP server...");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        lastSync = millis();
      }
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay 1 second
  }
}

/////////////////////SERVER THINGS///////////////////////

void handleSetAlarm() {
  if (server.hasArg("time") && server.hasArg("prealarm")) {
    String time = server.arg("time");
    int preAlarm = server.arg("prealarm").toInt();
    
    // Validate time format (HH:MM)
    if (time.length() == 5 && time.charAt(2) == ':') {
      int hour = time.substring(0, 2).toInt();
      int minute = time.substring(3, 5).toInt();
      
      // Validate hour and minute ranges
      if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && preAlarm > 0 && preAlarm <= 120) {
        // Save to memory
        alarmConfig.alarmTime = time;
        alarmConfig.Hour = hour;
        alarmConfig.Minute = minute;
        alarmConfig.preAlarmMinutes = preAlarm;
        alarmConfig.isSet = true;
        
        Serial.printf("Alarm set: %s with %d minutes pre-alarm\n", time.c_str(), preAlarm);
        server.send(200, "text/plain", "OK");
        startAlarm(); // Start the alarm task if not already running
      } else {
        server.send(400, "text/plain", "Invalid time or prealarm values");
      }
    } else {
      server.send(400, "text/plain", "Invalid time format. Use HH:MM");
    }
  } else {
    server.send(400, "text/plain", "Missing parameters: time and prealarm required");
  }
}

void handleStatus() {
  String response = "";
  
  if (alarmConfig.isSet) {
    response = "ALARM_SET:" + alarmConfig.alarmTime + ":" + String(alarmConfig.preAlarmMinutes);
  } else {
    response = "NO_ALARM";
  }
  
  server.send(200, "text/plain", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}


void startAlarm()
{
  time_t tempTime = currentTime;
  //calculate seconds to xx:00 minutes 
  int secondsToNextMinute = 60 - (localtime(&tempTime)->tm_sec);
  Serial.printf("Seconds to next full minute: %d\n", secondsToNextMinute);
  delay((secondsToNextMinute * 1000)-700); // Delay until next full minute
  tempTime = currentTime; // Update tempTime after delay

  //Calculate steps for led dimming up 
  int steps = 255 / alarmConfig.preAlarmMinutes;
  Serial.printf("Steps for dimming: %d\n", steps);

  //Calculate time to start pre-alarm - minutes after midnight
  int startTime = alarmConfig.Hour * 60 + alarmConfig.Minute - alarmConfig.preAlarmMinutes;
  Serial.printf("Start time for alarm: %d\n", startTime);

  int currentTimeInMinutes = (localtime(&tempTime)->tm_hour) * 60 + (localtime(&tempTime)->tm_min);
  Serial.printf("Current time in minutes: %d\n", currentTimeInMinutes);
  
  int toAlarmInMinutes{0};
  //calculate time to allarm in minutes 
  if(currentTimeInMinutes >= startTime) {
    toAlarmInMinutes = 24*60 - currentTimeInMinutes + startTime;
  } else {
    toAlarmInMinutes = startTime - currentTimeInMinutes;
  }
    Serial.println("toAlarmInMinutes value:" + String(toAlarmInMinutes));
    Serial.printf("Alarm should start in %d minutes\n so it is in %d hours and %d minutes \n", toAlarmInMinutes, toAlarmInMinutes / 60, toAlarmInMinutes % 60);
    delay(toAlarmInMinutes * 60000); // Delay until alarm time
    Serial.println("Starting pre-alarm sequence...");
    // Pre-alarm sequence: gradually increase LED brightness
    for (int brightness = steps; brightness < 255; brightness += steps) {
      analogWrite(LED_PIN, brightness);
      Serial.printf("LED brightness: %d\n", brightness);
      delay(60000); // Wait 1 minute between brightness increases
    }
    Serial.println("Pre-alarm sequence complete. LED at full brightness.");
    analogWrite(LED_PIN, 255); // Ensure LED is fully on
    delay(10000);
    for(int i=0; i<10; i++) {
      analogWrite(LED_PIN, 0); // Turn off LED
      delay(500);
      analogWrite(LED_PIN, 255); // Turn on LED
      delay(500);
    }
    analogWrite(LED_PIN, 255);
    delay(5 * 60 * 1000); // Keep LED on for 5 minutes
    analogWrite(LED_PIN, 0); // Turn off LED after alarm
    Serial.println("Alarm sequence complete. LED turned off.");
    alarmConfig.isSet = false; // Reset alarm 
}