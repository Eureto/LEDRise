#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration (extern declarations)
extern const char* ssid;
extern const char* password;

// Hardware configuration
extern const int LED_PIN;        // Pin connected to the LED
extern const int Internal_LED;   // Pin connected to the internal LED

// NTP server configuration for Poland (extern declarations)
extern const char* ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

// OTA update credentials 
extern const char* OTA_USERNAME;
extern const char* OTA_PASSWORD;

// Static IP configuration (change these to match your network)
#define STATIC_IP_ENABLED true  // Set to false for DHCP

#if STATIC_IP_ENABLED
  #define ESP32_IP      192, 168, 0, 150   // Your desired IP
  #define GATEWAY_IP    192, 168, 0, 1     // Your router IP
  #define SUBNET_MASK   255, 255, 255, 0   // Subnet mask
  #define PRIMARY_DNS   1, 1, 1, 1         // Cloudflare DNS
  #define SECONDARY_DNS 8, 8, 8, 8         // Google DNS
#endif

#endif