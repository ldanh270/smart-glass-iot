#pragma once
#include <Arduino.h>

// AudioOutI2S: wrapper đơn giản cho I2S output (dùng cho MAX98357A hoặc codec tương tự)
// - `begin()` khởi tạo driver I2S
// - `stop()` dừng phát hiện tại
// - `write()` gửi buffer PCM tới I2S
class AudioOutI2S {
public:
  // Khởi tạo driver I2S, trả về true nếu thành công
  bool begin();

  // Dừng/flush buffer (không un-install driver ở đây)
  void stop();

  // Ghi dữ liệu PCM (PMC: Pulse Modulation Code) (raw bytes) tới I2S. Trả về true nếu gửi thành công.
  // Kỳ vọng dữ liệu là PCM16 mono với sample rate định nghĩa trong config.
  bool write(const uint8_t* data, size_t len);
};
