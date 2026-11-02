---
tags: [ledrise, architecture]
---
# Architektura

```
 Telefon (Android)                         ESP32
┌───────────────────┐   HTTP (GET)   ┌────────────────────────────┐
│ MainActivity      │ ─────────────► │ WebServer :80              │
│ SettingsActivity  │ ◄───────────── │  /info /alarms /...        │
│ DeviceApi         │   JSON/tekst   │                            │
└───────────────────┘                │ alarmsTick() (loop, 1 Hz)  │
        │ skan /24                   │   └► sequenceTask (FreeRTOS)│
        └──► GET /info               │ NVS: alarmy + ustawienia   │
                                     │ SNTP + TZ_INFO             │
                                     └──────────┬─────────────────┘
                                                │ PWM (GPIO 25)
                                          MOSFET ► pasek LED
```

## Przepływ
1. Aplikacja ładuje zapamiętany adres lub skanuje sieć – patrz [[Wykrywanie urzadzenia]].
2. Zmiany alarmów idą przez [[API HTTP]]; ESP32 zapisuje je od razu w NVS.
3. `alarmsTick()` w pętli sprawdza zegar lokalny i startuje sekwencję – patrz [[Firmware ESP32]].
4. Zegar pochodzi z NTP, reguły stref z [[Czas i DST]].

## Decyzje projektowe
- **Konfiguracja na urządzeniu** – budzik działa bez telefonu i po restarcie.
- **Harmonogram oparty o zegar, nie o `delay()`** – odporny na DST i korekty NTP.
- **GET dla wszystkiego** – prosto, zgodnie z istniejącym stylem; sieć domowa, brak uwierzytelniania (poza OTA).
