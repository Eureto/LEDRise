---
tags: [ledrise, troubleshooting]
---
# Rozwiązywanie problemów

| Objaw | Przyczyna / rozwiązanie |
|---|---|
| „Device not found in this network” | telefon nie jest w tym samym Wi-Fi co ESP32, izolacja klientów na routerze, ESP nie połączył się z Wi-Fi (sprawdź Serial). Dotknij status, by wymusić skan. |
| Alarmy się nie odpalają, ostrzeżenie o zegarze | brak NTP (`timeValid:false`) – sprawdź dostęp ESP32 do internetu. Czas wraca sam po synchronizacji. |
| Wschód o godzinę przesunięty | sprawdź `TZ_INFO` ([[Czas i DST]]); powinno być `CET-1CEST,M3.5.0,M10.5.0/3`. |
| Alarm jednorazowy „zniknął” z aktywnych | to zamierzone – po odpaleniu jest wyłączany; włącz go przełącznikiem. |
| Nie można zmienić jasności/LED | trwa sekwencja budzika – użyj *Stop sunrise*. |
| Alarm nie startuje po restarcie w trakcie nocy | restart po zakończeniu okna wschodu = alarm przepadł; w oknie – kontynuuje. |
| Kompilacja: brak `config.cpp` | skopiuj z `config.cpp.template` ([[Budowanie i wgrywanie]]). |
