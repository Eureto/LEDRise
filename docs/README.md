---
tags: [ledrise, index]
---
# LedRise – dokumentacja

Budzik ze „wschodem słońca”: ESP32 steruje paskiem LED (MOSFET), aplikacja Android służy do konfiguracji. Folder `docs/` jest vaultem [Obsidian](https://obsidian.md) – otwórz go przez *Open folder as vault*.

## Spis treści
- [[Architektura]] – przegląd systemu i przepływ danych
- [[Hardware]] – części, pin, druk 3D
- [[Firmware ESP32]] – moduły, harmonogram, pamięć
- [[Czas i DST]] – strefa czasowa i zmiana czasu
- [[API HTTP]] – endpointy ESP32
- [[Wykrywanie urzadzenia]] – skanowanie sieci zamiast stałego IP
- [[Aplikacja Android]] – ekrany i kod
- [[Budowanie i wgrywanie]] – konfiguracja, kompilacja, OTA
- [[Rozwiazywanie problemow]]

## Najważniejsze zasady
- Konfiguracja (alarmy, ustawienia) **żyje na ESP32** w NVS i przeżywa restart. Aplikacja jest tylko edytorem.
- Urządzenie **nie ma stałego IP** – aplikacja znajduje je skanując sieć lokalną.
- Czas lokalny liczony według reguł CET/CEST, przejście na czas letni/zimowy jest automatyczne.
