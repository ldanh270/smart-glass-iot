#pragma once
#include <Arduino.h>

class AudioOutI2S {
public:
  bool begin();
  void stop();

  // Phát PCM16 mono 16kHz theo buffer
  bool write(const uint8_t* data, size_t len);
};
