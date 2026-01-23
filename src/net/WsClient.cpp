#include "net/WsClient.h"
#include "config.h"

#include "display/OledDisplay.h"
#include "tts/TtsClient.h"
#include "audio/AudioOutI2S.h"

#include <ArduinoJson.h>

String WsClient::safeTruncate(const String& s, size_t maxLen) const {
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen);
}

void WsClient::begin(OledDisplay& oled, TtsClient& tts, AudioOutI2S& audio) {
  oledPtr = &oled;
  ttsPtr = &tts;
  audioPtr = &audio;

  ws.onMessage([&](websockets::WebsocketsMessage msg) {
    // LƯU Ý: Kỳ vọng JSON như:
    // {"type":"translate_result","text":"...","lang":"vi"}
    JsonDocument doc; // LƯU Ý: ArduinoJson 7.x sử dụng JsonDocument thay vì StaticJsonDocument
    DeserializationError err = deserializeJson(doc, msg.data());
    if (err) return;

    const char* type = doc["type"] | "";
    if (String(type) != "translate_result") return;

    String text = doc["text"] | "";
    String lang = doc["lang"] | "vi";

    // LƯU Ý: Hiển thị ngay lập tức (phản hồi UI nhanh)
    oledPtr->printWrapped(safeTruncate(text, 300));

    // LƯU Ý: Phát (TTS phía server -> audio -> I2S)
    // Tránh chồng tiếng nếu kết quả tới nhanh
    if (!ttsPtr->isSpeaking()) {
      ttsPtr->requestAndPlay(safeTruncate(text, 300), lang, *audioPtr);
    }
  });

  ws.onEvent([&](websockets::WebsocketsEvent e, String data) {
    if (!oledPtr) return;
    if (e == websockets::WebsocketsEvent::ConnectionOpened) {
      oledPtr->printWrapped("WS Connected");
    } else if (e == websockets::WebsocketsEvent::ConnectionClosed) {
      oledPtr->printWrapped("WS Closed\nReconnecting...");
    }
  });

  ws.connect(WS_URL);
}

void WsClient::poll() {
  ws.poll();
}

bool WsClient::connected() const {
  // Kiểm tra kết nối - sử dụng non-const available()
  // Lưu ý: phương thức này không thể được const vì available() không const
  return const_cast<websockets::WebsocketsClient&>(ws).available();
}

void WsClient::ensureConnected() {
  if (connected()) return;

  unsigned long now = millis();
  if (now - lastReconnectAttemptMs > RECONNECT_INTERVAL_MS) {
    lastReconnectAttemptMs = now;
    ws.connect(WS_URL);
  }
}
