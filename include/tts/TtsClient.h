#pragma once
#include <Arduino.h>

class OledDisplay;
class AudioOutI2S;

// `TtsClient`: gửi yêu cầu TTS tới server, nhận stream audio và phát qua I2S.
// Thiết kế: stream trực tiếp từ HTTP response và đẩy PCM tới `AudioOutI2S`.
class TtsClient {
public:
  // Gửi `text` với `lang` tới endpoint TTS và phát trực tiếp qua `audio`.
  // Trả về true nếu quá trình phát bắt đầu/hoàn tất thành công.
  // Lưu ý: hàm này sẽ đọc stream liên tục nên có thể block trong thời gian phát.
  bool requestAndPlay(const String& text, const String& lang, AudioOutI2S& audio);

  // Trạng thái: đang phát hay không
  bool isSpeaking() const { return speaking; }

private:
  // Cờ nội bộ báo đang phát (true khi đang nhận/đẩy dữ liệu audio)
  bool speaking = false;

  // Kiểm tra xem buffer có giống header WAV (RIFF) hay không
  bool looksLikeWavHeader(const uint8_t* buf, size_t len) const;

  // Cắt chuỗi an toàn để giới hạn payload gửi tới server
  String safeTruncate(const String& s, size_t maxLen) const;
};
