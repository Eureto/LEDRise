#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration (extern declarations)
extern const char* ssid;
extern const char* password;

// Hardware configuration
#define LED_PIN 33
#define Internal_LED 33

// NTP server configuration for Poland (extern declarations)
extern const char* ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

#endif