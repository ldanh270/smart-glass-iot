#pragma once
#include <Arduino.h>

class OledDisplay;
class AudioOutI2S;

class TtsClient {
public:
  // stream TTS từ server -> phát I2S
  bool requestAndPlay(const String& text, const String& lang, AudioOutI2S& audio);

  bool isSpeaking() const { return speaking; }

private:
  bool speaking = false;

  bool looksLikeWavHeader(const uint8_t* buf, size_t len) const;
  String safeTruncate(const String& s, size_t maxLen) const;
};
