#include "tts/TtsClient.h"
#include "config.h"
#include "audio/AudioOutI2S.h"


#include <WiFi.h>
#include <HTTPClient.h>

String TtsClient::safeTruncate(const String& s, size_t maxLen) const {
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen);
}

bool TtsClient::looksLikeWavHeader(const uint8_t* buf, size_t len) const {
  if (len < 12) return false;
  return (buf[0]=='R' && buf[1]=='I' && buf[2]=='F' && buf[3]=='F');
}

bool TtsClient::requestAndPlay(const String& text, const String& lang, AudioOutI2S& audio) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(TTS_URL);
  http.addHeader("Content-Type", "application/json");

  // LƯU Ý: giữ payload nhỏ; nếu text quá dài, cắt ngắn để an toàn
  String t = safeTruncate(text, 300);

  String payload;
  payload.reserve(256);
  payload = "{\"text\":\"";
  // Escape JSON cơ bản cho dấu ngoặc/dấu gạch chéo (tối thiểu)
  for (size_t i = 0; i < t.length(); i++) {
    char ch = t[i];
    if (ch == '\\' || ch == '"') payload += '\\';
    payload += ch;
  }
  payload += "\",\"lang\":\"";
  payload += lang;
  payload += "\"}";

  int code = http.POST(payload);
  if (code <= 0) { http.end(); return false; }
  if (code != 200) { http.end(); return false; }

  WiFiClient* stream = http.getStreamPtr();

  speaking = true;

  const size_t BUF_SZ = 1024;
  uint8_t buf[BUF_SZ];

  // Kiểm tra header WAV (nếu server trả file WAV) và bỏ qua header 44 byte
  bool headerChecked = false;
  bool skippedWavHeader = false;
  size_t wavSkipLeft = 44;

  // Thời gian để phát hiện mất dữ liệu; nếu vượt quá sẽ thoát vòng đọc
  unsigned long lastDataMs = millis();

  // Đọc stream cho tới khi server đóng hoặc có lỗi
  while (http.connected()) {
    int avail = stream->available();
    if (avail <= 0) {
      // Tránh busy-loop, đợi một chút
      delay(5);
      if (millis() - lastDataMs > 5000) break; // timeout nếu không có dữ liệu
      continue;
    }

    int toRead = min(avail, (int)BUF_SZ);
    int n = stream->readBytes(buf, toRead);
    if (n <= 0) continue;

    lastDataMs = millis();

    // Chỉ kiểm tra header một lần đầu tiên
    if (!headerChecked) {
      headerChecked = true;
      if (looksLikeWavHeader(buf, n)) skippedWavHeader = true;
    }

    size_t offset = 0;
    // Nếu là WAV, bỏ qua phần header 44 byte trước khi gửi PCM thực
    if (skippedWavHeader && wavSkipLeft > 0) {
      size_t skipNow = min((size_t)n, wavSkipLeft);
      offset += skipNow;
      wavSkipLeft -= skipNow;
      if (offset >= (size_t)n) continue; // tất cả dữ liệu hiện tại là header
    }

    // Ghi phần PCM còn lại tới I2S. Nếu ghi thất bại thì dừng phát.
    if (!audio.write(buf + offset, (size_t)n - offset)) break;
  }

  // Dọn dẹp: dừng I2S, cập nhật trạng thái và đóng kết nối HTTP
  audio.stop();
  speaking = false;

  http.end();
  return true;
}
