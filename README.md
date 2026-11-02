Created using PlatformIO for VSCode 
and Android Studio

Componenets I used:
- Esp32 Wroom32D
- White LED strip aprox 1.5m
- 12v, 2A PSU
- Lm2596 DC-DC converter
- IRLB8721 MOSFET
- Resistors(TODO: Values)
- Switch, breadboard... 


TODO:
  

Printing
- Part2 needs to be a little flexible so keep that in mind when printing. I used PETG 

COMPILING:
- Create config.cpp from config.cpp.template
- Set WiFi credentials in config.cpp. The ESP32 uses DHCP, the app finds it by scanning the local /24 subnet for `GET /info` -> `{"device":"LedRise"}` (no fixed IP needed).
- Alarms and settings are stored on the ESP32 (NVS flash) and survive restarts; the app only edits them.
- Timezone/DST rules: `TZ_INFO` in config.h (Poland: CET/CEST, switches automatically).

Dokumentacja (vault Obsidian): [docs/README.md](docs/README.md)
