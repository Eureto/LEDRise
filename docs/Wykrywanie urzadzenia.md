---
tags: [ledrise, discovery, android]
---
# Wykrywanie urządzenia

Urządzenie używa DHCP, więc adres może się zmienić. Aplikacja nie ma wpisanego IP.

## Algorytm (`DeviceApi`)
1. `get(path)` próbuje zapamiętanego hosta (`SharedPreferences`, klucz `deviceHost`).
2. Przy błędzie połączenia uruchamia `discover()`:
   - z `ConnectivityManager` bierze adres IPv4 Wi-Fi telefonu,
   - buduje listę `a.b.c.1–254` (bez własnego adresu) – skanowana jest podsieć /24,
   - równolegle (semafor 48) wysyła `GET /info` z krótkimi timeoutami (500/800 ms),
   - pierwszy host zwracający `"device":"LedRise"` wygrywa, reszta jest anulowana.
3. Znaleziony adres jest zapamiętany, żądanie powtórzone.

W UI: dotknięcie paska statusu na ekranie głównym lub „Find device again” w ustawieniach wymusza skan.

## Wymagania
- uprawnienia `INTERNET`, `ACCESS_NETWORK_STATE`,
- `network_security_config` z `cleartextTrafficPermitted="true"` (HTTP, brak TLS),
- telefon i ESP32 w tej samej podsieci (izolacja klientów na routerze/gościnne Wi-Fi uniemożliwia skan).

Endpoint opisany w [[API HTTP]].
