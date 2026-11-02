---
tags: [ledrise, android]
---
# Aplikacja Android

Kotlin, widoki XML, Material 3, OkHttp. `minSdk 29`, pakiet `com.ledrise`.

## Pliki
| Plik | Rola |
|---|---|
| `DeviceApi.kt` | klient HTTP + [[Wykrywanie urzadzenia]], modele `Alarm`, `DeviceInfo` |
| `MainActivity.kt` | lista alarmów, dialog edycji, sterowanie LED |
| `SettingsActivity.kt` | czas świecenia i mignięcia (zapis na ESP32) |
| `res/layout/item_alarm.xml` | wiersz alarmu (godzina, dni, przełącznik) |
| `res/layout/dialog_alarm_edit.xml` | dialog: godzina, długość wschodu, dni, powtarzanie |

## Ekran główny
- lista alarmów posortowana po godzinie; przełącznik włącza/wyłącza alarm,
- dotknięcie wiersza → edycja (z przyciskiem *Delete*),
- *Add alarm*, *Stop sunrise* ([[API HTTP]] `/stopalarm`),
- włącznik LED i suwak jasności (żądania ograniczone do ~1 na 150 ms),
- pasek statusu: adres urządzenia, ostrzeżenie o niezsynchronizowanym zegarze, informacja o trwającym wschodzie.

## Dialog alarmu
Dni Pn–Nd (przyciski przełączane). *Repeat every week* wyłączone = alarm jednorazowy w najbliższy wybrany dzień.

## Ustawienia
Wartości są czytane z `/info` i zapisywane przy wyjściu. Jeśli urządzenia nie znaleziono, ekran zamyka się bez zapisu.

## Znane uwagi
- Teksty interfejsu są po angielsku, wpisane na stałe w layoutach/kodzie.
- Brak uwierzytelniania żądań – zakładana zaufana sieć domowa.
