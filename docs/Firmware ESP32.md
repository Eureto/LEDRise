---
tags: [ledrise, firmware]
---
# Firmware ESP32

PlatformIO, framework Arduino (`ESP32Code/`). Zależność: ElegantOTA.

## Moduły
| Plik | Rola |
|---|---|
| `main.cpp` | `setup()`: `alarmsBegin` → Wi-Fi → czas → serwer; `loop()`: obsługa serwera, OTA, `alarmsTick()` |
| `alarm.cpp/.h` | model alarmu, NVS, harmonogram, sekwencja wschodu |
| `web_Server.cpp/.h` | endpointy – [[API HTTP]] |
| `rtc.cpp` | NTP + strefa – [[Czas i DST]] |
| `wifiConnection.cpp` | łączenie z Wi-Fi (DHCP) |
| `ledSignals.cpp` | miganie diodą statusu |
| `debuggingShit.cpp` | wypisywanie czasu na Serial |
| `config.cpp` / `config.h` | piny, hasła (`config.cpp` poza gitem, wzór: `config.cpp.template`) |

## Model alarmu
```cpp
struct Alarm {
  uint8_t used, hour, minute;
  uint8_t preMinutes;  // 1-120, długość wschodu
  uint8_t days;        // bit0 = niedziela ... bit6 = sobota
  uint8_t repeat;      // 1 = co tydzień, 0 = jednorazowo
  uint8_t enabled;
};
```
Indeks w tablicy (0–19) jest identyfikatorem alarmu.

## Pamięć trwała
Namespace NVS `ledrise`:
- `alarms` – cała tablica `Alarm[MAX_ALARMS]` (blob), walidowana przy odczycie,
- `ledOnMin` – ile minut LED świeci po alarmie,
- `flashes` – liczba mignięć po wschodzie.

## Harmonogram
`alarmsTick()` (raz na sekundę, nic nie robi dopóki zegar niezsynchronizowany, `time < MIN_VALID_TIME`):
1. Dla każdego aktywnego alarmu bierze dziś i jutro (wschód może zacząć się dzień wcześniej).
2. Buduje czas alarmu przez `mktime()` (czas lokalny, `tm_isdst=-1`) – stąd poprawność przy DST.
3. Jeśli `start ≤ teraz < alarm` i dany alarm nie był jeszcze odpalony → `fireAlarm`.
4. Alarm jednorazowy jest wyłączany i zapisywany w momencie startu.

## Sekwencja (`sequenceTask`)
1. **Wschód:** jasność = `(teraz − start) / (alarm − start) · 255`, liczona z zegara. Po restarcie w środku okna sekwencja startuje od właściwej jasności.
2. 10 s pełnej jasności.
3. Mignięcia (`flashingRepetitions` × 100 ms wył./wł.).
4. Pełna jasność przez `ledOnMinutes`, potem wyłączenie.

Sekwencję można przerwać (`/stopalarm`) – flaga `stopRequested` sprawdzana w każdej pauzie. Podczas sekwencji ręczne sterowanie LED jest blokowane.

## Ograniczenia
- Alarm wyznaczony na godzinę, która nie istnieje (przeskok wiosną 02:00→03:00), zachowuje się według `mktime()`.
- Jeśli ESP było wyłączone w całym oknie wschodu, alarm przepada.
