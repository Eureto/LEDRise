---
tags: [ledrise, hardware]
---
# Hardware

## Części
- ESP32 WROOM-32D
- biały pasek LED ok. 1.5 m
- zasilacz 12 V / 2 A
- przetwornica DC-DC LM2596 (zasilanie ESP32)
- MOSFET IRLB8721 (sterowanie paskiem)
- rezystory (wartości: TODO), przełącznik, płytka stykowa

## Piny
| Funkcja | GPIO | Źródło |
|---|---|---|
| Pasek LED (PWM) | 25 | `LED_PIN` w `config.cpp` |
| Dioda sygnalizacyjna | 25 | `Internal_LED` (ten sam pin) |

Sygnalizacja stanu (łączenie z Wi-Fi, synchronizacja czasu) miga tym samym pinem co pasek, więc podczas startu pasek świeci słabo.

## Druk 3D
`Sterownik - Part 1.stl`, `Sterownik - Part 2.stl`. Część 2 musi być lekko elastyczna – użyto PETG.
