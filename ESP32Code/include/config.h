#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration (extern declarations)
extern const char* ssid;
extern const char* password;

// Hardware configuration
#define LED_PIN 25
#define Internal_LED 25

// NTP server configuration for Poland (extern declarations)
extern const char* ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

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