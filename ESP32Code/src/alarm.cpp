#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "alarm.h"
#include "ledSignals.h"

AlarmConfig alarmConfig = {"", 0, 0, 0, false};


int calculatePauseTimeInMilliseconds(int preAlarmMinutes){
    int steps = 255;
    int milisecondsPerStep = (preAlarmMinutes * 60000) / steps;
    Serial.printf("Calculated miliseconds per step: %d\n", milisecondsPerStep);
    return milisecondsPerStep;
}

// Staring pre-alarm and alarm sequence
void startAlarm()
{
    time_t tempTime = time(nullptr);
    //calculate seconds to xx:00 minutes 
    int secondsToNextMinute = 60 - (localtime(&tempTime)->tm_sec);
    Serial.printf("Seconds to next full minute: %d\n", secondsToNextMinute);
    delay((secondsToNextMinute + 0.1) * 1000); // Delay until next full minute (+0.1 to ensure we are past the full minute)
    tempTime = time(nullptr); // Update tempTime after delay

    // Getting values that are needed to determinate when to start alarm
    // startClockTimeInMinutes is the time in minutes after midnight when pre-alarm should start 
    int startClockTimeInMinutes = alarmConfig.Hour * 60 + alarmConfig.Minute - alarmConfig.preAlarmMinutes;
    Serial.printf("Start time for alarm: %d\n", startClockTimeInMinutes);

    // Gets current time in minutes after midnight
    int currentTimeInMinutes = (localtime(&tempTime)->tm_hour) * 60 + (localtime(&tempTime)->tm_min);
    Serial.printf("Current time in minutes: %d\n", currentTimeInMinutes);

    // We need to check if alarm will start this day or the next day
    // then calculate minutes to alarm "toAlarmInMinutes"
    int toAlarmInMinutes{0};
    if(currentTimeInMinutes >= startClockTimeInMinutes) {
    toAlarmInMinutes = 24*60 - currentTimeInMinutes + startClockTimeInMinutes;
    } else {
    toAlarmInMinutes = startClockTimeInMinutes - currentTimeInMinutes ;
    }
    Serial.printf("Alarm should start in %d minutes\n so it is in %d hours and %d minutes \n", toAlarmInMinutes, toAlarmInMinutes / 60, toAlarmInMinutes % 60);
    
    // Start delay until pre alarm sequence should start
    delay(toAlarmInMinutes * 60000); 
    
    
    //////////////////////////// PRE ALARM SEQUENCE ////////////////////////////
    // Pre-alarm sequence: gradually increase LED brightness
    
    int brightness = 0;
    int pauseTime = calculatePauseTimeInMilliseconds(alarmConfig.preAlarmMinutes);
    Serial.println("Starting pre-alarm sequence...");
    
    while(brightness < 255){
        analogWrite(LED_PIN, brightness);
        Serial.printf("LED brightness: %d\n", brightness);
        delay(pauseTime); // Wait calculated time between brightness increases
        brightness += 1;
    }
    Serial.println("Pre-alarm sequence complete. LED at full brightness.");
    
    
    //////////////////////////// ALARM SEQUENCE //////////////////////////// 
    
    analogWrite(LED_PIN, 255); // Ensure LED is fully on
    delay(10000); 
    LedBlinkingParams ledBlinkingParams = {100, 100}; // Fast blinking parameters
    ledSignals(&ledBlinkingParams, 30); // CAUTION: This function uses INTERNAL_LED pin as output.
    
    analogWrite(LED_PIN, 255);
    delay(5 * 60 * 1000); // Keep LED on for 5 minutes
    analogWrite(LED_PIN, 0); // Turn off LED after alarm
    Serial.println("Alarm sequence complete. LED turned off.");
    alarmConfig.isSet = false; // Reset alarm 
}

