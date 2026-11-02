---
tags: [ledrise, time]
---
# Czas i DST

## Był błąd
`configTime(3600, 3600, …)` ustawiało stałe UTC+2 przez cały rok (offset strefy + offset letni), a stary alarm liczył jedno długie `delay()` – po zmianie czasu godzina była przesunięta.

## Jak jest teraz
```cpp
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov", "0.pool.ntp.org");
```
- CET = UTC+1, CEST = UTC+2
- start DST: ostatnia niedziela marca, 02:00 → 03:00
- koniec DST: ostatnia niedziela października, 03:00 → 02:00

Biblioteka C (`localtime`, `mktime`) sama stosuje reguły, więc [[Firmware ESP32#Harmonogram|harmonogram]] liczący czas alarmu w czasie lokalnym zawsze trafia w godzinę ustawioną w aplikacji.

## Synchronizacja
- `initializeTime()` czeka do 2 minut na NTP przy starcie (dioda świeci długo/krótko).
- Brak fałszywego czasu zastępczego: gdy NTP zawiedzie, alarmy są nieaktywne, a SNTP dalej próbuje w tle. `/info` zwraca wtedy `timeValid:false`, a aplikacja pokazuje ostrzeżenie.
- Zmiana strefy: edytuj `TZ_INFO` w `config.h` ([lista POSIX TZ](https://github.com/nayarsystems/posix_tz_db)).
