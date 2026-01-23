#include "net/WsClient.h"
#include "config.h"

#include "display/OledDisplay.h"
#include "tts/TtsClient.h"
#include "audio/AudioOutI2S.h"

#include <ArduinoJson.h>

// Hàm tiện ích: cắt chuỗi an toàn để tránh gửi/hiển thị quá dài
String WsClient::safeTruncate(const String& s, size_t maxLen) const {
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen);
}

// Khởi tạo WsClient: lưu tham chiếu tới các service phụ trợ và đăng ký callback
void WsClient::begin(OledDisplay& oled, TtsClient& tts, AudioOutI2S& audio) {
  // Lưu con trỏ để có thể gọi các chức năng hiển thị / TTS / audio từ callback
  oledPtr = &oled;
  ttsPtr = &tts;
  audioPtr = &audio;

  // Đăng ký xử lý message WebSocket
  ws.onMessage([&](websockets::WebsocketsMessage msg) {
    // Kỳ vọng payload là JSON, ví dụ:
    // {"type":"translate_result","text":"...","lang":"vi"}
    JsonDocument doc; // ArduinoJson v7: dùng JsonDocument động
    DeserializationError err = deserializeJson(doc, msg.data());
    if (err) return; // nếu JSON không hợp lệ, bỏ qua

    const char* type = doc["type"] | "";
    if (String(type) != "translate_result") return; // chỉ xử lý loại này

    // Lấy text và ngôn ngữ (mặc định "vi")
    String text = doc["text"] | "";
    String lang = doc["lang"] | "vi";

    // Hiển thị ngay lập tức trên OLED để phản hồi UI nhanh
    oledPtr->printWrapped(safeTruncate(text, 300));

    // Yêu cầu TTS và phát qua audio nếu hiện tại không đang phát
    // (tránh chồng tiếng nếu nhiều message tới nhanh)
    if (!ttsPtr->isSpeaking()) {
      ttsPtr->requestAndPlay(safeTruncate(text, 300), lang, *audioPtr);
    }
  });

  // Đăng ký sự kiện kết nối/đóng kết nối để cập nhật UI
  ws.onEvent([&](websockets::WebsocketsEvent e, String data) {
    if (!oledPtr) return;
    if (e == websockets::WebsocketsEvent::ConnectionOpened) {
      oledPtr->printWrapped("WS Connected");
    } else if (e == websockets::WebsocketsEvent::ConnectionClosed) {
      oledPtr->printWrapped("WS Closed\nReconnecting...");
    }
  });

  // Kết nối tới server WebSocket (non-blocking tùy lib)
  ws.connect(WS_URL);
}

// Gọi poll để xử lý sự kiện WebSocket (phải gọi trong loop())
void WsClient::poll() {
  ws.poll();
}

// Trả về trạng thái kết nối. Lưu ý: `available()` của thư viện không phải const,
// nên ở đây ta dùng const_cast để gọi nó từ phương thức const.
bool WsClient::connected() const {
  return const_cast<websockets::WebsocketsClient&>(ws).available();
}

// Nếu không kết nối, thử reconnect theo khoảng thời gian quy định
void WsClient::ensureConnected() {
  if (connected()) return;

  unsigned long now = millis();
  if (now - lastReconnectAttemptMs > RECONNECT_INTERVAL_MS) {
    lastReconnectAttemptMs = now;
    ws.connect(WS_URL);
  }
}
