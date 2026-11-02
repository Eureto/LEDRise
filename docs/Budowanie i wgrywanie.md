---
tags: [ledrise, build]
---
# Budowanie i wgrywanie

## Firmware
1. Skopiuj `ESP32Code/src/config.cpp.template` → `config.cpp` (plik jest w `.gitignore`) i uzupełnij SSID, hasło Wi-Fi, dane OTA.
2. (Opcjonalnie) zmień strefę czasu: `TZ_INFO` w `include/config.h` – [[Czas i DST]].
3. `pio run -t upload` (lub przycisk w VS Code/PlatformIO).
4. Kolejne aktualizacje: OTA przez `http://<adres>/update` (login z `config.cpp`). Adres zobaczysz w Serial Monitor (115200) albo w aplikacji (pasek statusu).

> Sekcja `STATIC_IP_*` w `config.h` jest nieużywana – ESP32 działa na DHCP.

## Aplikacja
- Android Studio → otwórz `MobileApp/`, uruchom `app`.
- CLI: `./gradlew assembleDebug` (Gradle 8.13, AGP 8.13).
- `local.properties` jest lokalny – ustaw własne `sdk.dir`.
- Uwaga: `gradlew` ma końcówki linii CRLF; na Linuksie użyj `dos2unix gradlew` lub zainstalowanego Gradle 8.13.

## Test po wgraniu
1. Uruchom aplikację – powinna znaleźć urządzenie sama.
2. Dodaj alarm za kilka minut z krótkim wschodem (np. 2 min) – LED powinien rozjaśniać się do godziny alarmu.
3. Zrestartuj ESP32 – alarm ma pozostać na liście.
4. Sprawdź alarm tuż przed zmianą czasu (ostatnia niedziela marca/października).
