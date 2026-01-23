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

  bool headerChecked = false;
  bool skippedWavHeader = false;
  size_t wavSkipLeft = 44;

  // Hết thời gian chờ an toàn để tránh bị kẹt phát nếu server dừng
  unsigned long lastDataMs = millis();

  while (http.connected()) {
    int avail = stream->available();
    if (avail <= 0) {
      // LƯU Ý: độ trễ nhỏ ngăn chặn chiếm dụng CPU
      delay(5);
      if (millis() - lastDataMs > 5000) break;
      continue;
    }

    int toRead = min(avail, (int)BUF_SZ);
    int n = stream->readBytes(buf, toRead);
    if (n <= 0) continue;

    lastDataMs = millis();

    // Kiểm tra WAV header một lần (nếu server trả WAV)
    if (!headerChecked) {
      headerChecked = true;
      if (looksLikeWavHeader(buf, n)) skippedWavHeader = true;
    }

    size_t offset = 0;
    if (skippedWavHeader && wavSkipLeft > 0) {
      size_t skipNow = min((size_t)n, wavSkipLeft);
      offset += skipNow;
      wavSkipLeft -= skipNow;
      if (offset >= (size_t)n) continue;
    }

    // Ghi PCM còn lại tới I2S
    if (!audio.write(buf + offset, (size_t)n - offset)) break;
  }

  audio.stop();
  speaking = false;

  http.end();
  return true;
}
