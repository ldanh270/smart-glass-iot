#pragma once
#include <Arduino.h>
#include <ArduinoWebsockets.h>

class OledDisplay;
class TtsClient;
class AudioOutI2S;

class WsClient {
public:
  void begin(OledDisplay& oled, TtsClient& tts, AudioOutI2S& audio);
  void poll();
  bool connected() const;

  void ensureConnected(); // reconnect có giới hạn

private:
  websockets::WebsocketsClient ws;
  OledDisplay* oledPtr = nullptr;
  TtsClient* ttsPtr = nullptr;
  AudioOutI2S* audioPtr = nullptr;

  unsigned long lastReconnectAttemptMs = 0;
  static const unsigned long RECONNECT_INTERVAL_MS = 2000;

  String safeTruncate(const String& s, size_t maxLen) const;
};
