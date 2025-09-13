#include "config.h"

// WiFi configuration definitions
const char* ssid = "";
const char* password = "";

// NTP server configuration for Poland definitions
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 1 * 3600;  // Poland is UTC+1
const int daylightOffset_sec = 3600;  // DST offset (summer time)
