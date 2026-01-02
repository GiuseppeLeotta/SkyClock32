#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <time.h>
#else
#include <WiFi.h>
#include <sys/time.h>
#endif
#include <WiFiUdp.h>

#define NTP_ERR_WIFI_OFF -3
#define NTP_ERR_WIFI_ERR -2
#define NTP_ERR_TIMEOUT -1
#define NTP_WAIT 0
#define NTP_OK 1


class NTPClientNB {
public:
  // server = hostname NTP, port = 123 normalmente, syncInterval = ms tra sync
  // responseTimeout = ms da attendere prima di considerare fallita la richiesta
  NTPClientNB(const char* server = "pool.ntp.org", uint16_t port = 123,
              uint32_t syncInterval = 3600000, uint32_t responseTimeout = 5000)
    : _server(server), _port(port), _syncInterval(syncInterval), _lastSync(0), _wifiOff(false),
      _responseTimeout(responseTimeout), _lastSyncMs(0), _txMs(0) {}

  void begin(bool wifiOff=false) {
    _wifiOff=wifiOff;
    if (!_wifiOff) _udp.begin(2390);  // porta locale arbitraria
  }
  
  time_t lastSync() const { 
    return _lastSync;
  }

  // Chiamare spesso in loop(): ritorna true se ha appena sincronizzato
  int update() {
    if (_wifiOff) return NTP_ERR_WIFI_OFF; 
    uint32_t nowMs = millis();

    // gestione timeout: se in attesa e superato il timeout, reset TX per reinviare
    if (_txMs != 0 && (nowMs - _txMs >= _responseTimeout)) {
      //Serial.println("NTP request timeout, reinvio in corso...");
      _txMs = 0;
      _lastSyncMs = 0;  // prepara nuovo invio
      return NTP_ERR_TIMEOUT;
    }

    

    // invia nuova richiesta se non in attesa e scaduto intervallo
    if (_txMs == 0 && ((nowMs - _lastSyncMs >= _syncInterval) || _lastSyncMs == 0)) {
      if (WiFi.status() != WL_CONNECTED) return NTP_ERR_WIFI_ERR;
      if (sendPacket()) {
        _txMs = nowMs;
      }
    }

    // controlla arrivo pacchetto NTP
    int cb = _udp.parsePacket();
    if (cb >= 48 && _txMs != 0) {
      uint8_t buf[48];
      _udp.read(buf, 48);
      uint32_t rxMs = millis();
      uint32_t roundTrip = rxMs - _txMs;

      // estrai timestamp (secondi) byte 40-43
      uint32_t secondsSince1900 =
        (uint32_t(buf[40]) << 24) | (uint32_t(buf[41]) << 16) | (uint32_t(buf[42]) << 8) | (uint32_t(buf[43]) << 0);

      // converti in epoch UNIX e passa a ms
      const uint32_t seventyYears = 2208988800UL;
      uint32_t t = secondsSince1900 - seventyYears;
      uint64_t tMs = uint64_t(t) * 1000ULL;

      // calcola timestamp corretto in ms, aggiungendo metà round-trip
      uint64_t correctedMs = tMs + (roundTrip / 2);
      time_t correctedSec = time_t(correctedMs / 1000ULL);
      suseconds_t correctedUSec = suseconds_t((correctedMs % 1000ULL) * 1000ULL);

      // aggiorna l'orologio di sistema
      struct timeval tv;
      tv.tv_sec = correctedSec;
      tv.tv_usec = correctedUSec;
      settimeofday(&tv, nullptr);

      _lastSync=correctedSec;
      _lastSyncMs = rxMs;
      _txMs = 0;
      return NTP_OK;
    }

    return NTP_WAIT;
  }
private:
  const char* _server;
  uint16_t _port;
  uint32_t _syncInterval;
  uint32_t _responseTimeout;
  time_t _lastSync;
  bool _wifiOff;

  WiFiUDP _udp;
  uint32_t _lastSyncMs;
  uint32_t _txMs;
  int32_t _timeOffsetSec;

  bool sendPacket() {
    // costruisci pacchetto NTP di 48 byte
    uint8_t packet[48] = { 0 };
    packet[0] = 0b11100011;  // LI=0, VN=4, Mode=3 (client)

    _udp.beginPacket(_server, _port);
    _udp.write(packet, 48);
    return (_udp.endPacket() == 1);
  }
};
