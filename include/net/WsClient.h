#pragma once
#include <Arduino.h>
#include <ArduinoWebsockets.h>

// Tiền khai báo để tránh include vòng giữa các header
class OledDisplay;
class TtsClient;
class AudioOutI2S;

// WsClient: wrapper đơn giản quanh `websockets::WebsocketsClient`
// - Đăng ký callback để nhận message JSON từ server
// - Kết hợp với `OledDisplay`, `TtsClient`, `AudioOutI2S` để hiển thị và phát âm
class WsClient {
public:
  // Gọi khi khởi tạo, truyền tham chiếu tới các service phụ trợ
  void begin(OledDisplay& oled, TtsClient& tts, AudioOutI2S& audio);

  // Gọi định kỳ trong `loop()` để xử lý event
  void poll();

  // Kiểm tra trạng thái kết nối
  bool connected() const;

  // Nếu rớt kết nối, cố reconnect theo khoảng thời gian có giới hạn
  void ensureConnected();

private:
  websockets::WebsocketsClient ws; // client WebSocket thực tế

  // Con trỏ tới các service để gọi khi có message
  OledDisplay* oledPtr = nullptr;
  TtsClient* ttsPtr = nullptr;
  AudioOutI2S* audioPtr = nullptr;

  // Quản lý thời gian reconnect để tránh spam reconnect liên tục
  unsigned long lastReconnectAttemptMs = 0;
  static const unsigned long RECONNECT_INTERVAL_MS = 2000;

  // Tiện ích: cắt chuỗi dài
  String safeTruncate(const String& s, size_t maxLen) const;
};
