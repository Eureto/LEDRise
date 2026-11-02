---
tags: [ledrise, api]
---
# API HTTP

Serwer na porcie 80. Wszystko przez `GET`. Błędy: kod 4xx + tekst. Implementacja: `web_Server.cpp`. Klient: `DeviceApi.kt` ([[Aplikacja Android]]).

## Informacje / wykrywanie
### `GET /info`
Służy do [[Wykrywanie urzadzenia]] i do odczytu stanu.
```json
{"device":"LedRise","version":2,"ip":"192.168.0.23","mac":"..",
 "time":"2026-11-02 20:27:00","timeValid":true,"ledOn":false,
 "sequenceRunning":false,"ledOnMinutes":5,"flashingRepetitions":0,"maxAlarms":20}
```

## Alarmy
### `GET /alarms`
```json
{"alarms":[{"id":0,"hour":7,"minute":0,"pre":15,"days":62,"repeat":true,"enabled":true}]}
```
`days`: maska, bit0 = niedziela … bit6 = sobota (np. pn–pt = 62).

### `GET /alarms/set`
| Parametr | Opis |
|---|---|
| `time` | `HH:MM` |
| `prealarm` | 1–120 minut |
| `days` | maska 1–127 |
| `repeat` | `1` co tydzień (domyślnie), `0` jednorazowo |
| `enabled` | `1`/`0` (domyślnie 1) |
| `id` | opcjonalnie; brak = nowy alarm w wolnym slocie |

Odpowiedź: JSON zapisanego alarmu. Błędy: 400 (parametry, limit alarmów), 404 (nieznane `id`).

### `GET /alarms/delete?id=N`
`OK` lub 404.

### `GET /stopalarm`
Przerywa trwającą sekwencję (konfiguracja zostaje). 400 jeśli nic nie trwa.

## Ustawienia (zapisywane w NVS)
- `GET /ledonminutes?minutes=0..60`
- `GET /setflashing?repetitions=0..10000`

## Ręczne sterowanie LED
- `GET /turnOnOffLed` → `LED turned on` / `LED turned off`
- `GET /ledStateStatus` → `ON` / `OFF`
- `GET /setBrightness?value=0..255`

Podczas sekwencji budzika te trzy zwracają 400.

## OTA
`/update` (ElegantOTA, dane z `OTA_USERNAME`/`OTA_PASSWORD`) – patrz [[Budowanie i wgrywanie]].

## Usunięte
`/setalarm`, `/alarmStatus` (zastąpione przez `/alarms*`).
