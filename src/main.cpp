#include <Arduino.h>

// Thư viện / module của dự án:
// - `OledDisplay`: xử lý hiển thị OLED (in chữ, cuộn, wrap...)
// - `AudioOutI2S`: xuất audio qua I2S (DAC / codec)
// - `WiFiManager`: quản lý kết nối WiFi (connectBlocking, isConnected)
// - `WsClient`: client WebSocket để nhận lệnh/tin nhắn
// - `TtsClient`: client Text-To-Speech (chuyển text -> audio)
#include "display/OledDisplay.h"
#include "audio/AudioOutI2S.h"
#include "net/WiFiManager.h"
#include "net/WsClient.h"
#include "tts/TtsClient.h"

// Các đối tượng toàn cục cho các thành phần phần cứng / dịch vụ.
// Dùng static ở đây để đảm bảo vùng nhớ tồn tại trong toàn bộ vòng đời chương trình.
static OledDisplay oled;
static AudioOutI2S audio;
static WiFiManager wifi;
static WsClient wsClient;
static TtsClient tts;

// Hàm khởi tạo, chạy một lần khi board bật
void setup() {
  // Khởi tạo giao tiếp Serial để debug
  Serial.begin(115200);

  // Khởi tạo OLED. Nếu fail thì dừng (lỗi cứng), vì thiết bị cần hiển thị
  if (!oled.begin()) {
    // Vòng lặp vô hạn để báo lỗi phần cứng (thường có đèn/error handler bên ngoài)
    while (true) { delay(1000); } // OLED lỗi cứng
  }

  // Khởi tạo I2S / audio. Nếu fail thì in lỗi và dừng (không thể phát âm thanh)
  if (!audio.begin()) {
    oled.printWrapped("I2S init FAIL");
    while (true) { delay(1000); } // I2S lỗi cứng
  }

  // Kết nối WiFi trước khi sử dụng các dịch vụ mạng
  oled.printWrapped("WiFi connecting...");
  // `connectBlocking()` sẽ chặn cho đến khi kết nối hoặc timeout nội bộ
  wifi.connectBlocking();
  oled.printWrapped("WiFi OK");

  // Bắt đầu client WebSocket, truyền tham chiếu tới các đối tượng cần thiết
  // để `wsClient` có thể hiển thị thông báo, phát TTS và phát audio.
  wsClient.begin(oled, tts, audio);
}

// Vòng lặp chính, chạy liên tục
void loop() {
  // Poll sự kiện WebSocket: nhận message, xử lý callback, v.v.
  wsClient.poll();

  // Nếu kết nối WS bị rớt, `ensureConnected()` sẽ cố reconnect theo chính sách
  // nội bộ của `WsClient` (thường có giới hạn và backoff).
  wsClient.ensureConnected();

  // Kiểm tra WiFi: nếu rớt, cố reconnect (đơn giản, blocking)
  if (!wifi.isConnected()) {
    oled.printWrapped("WiFi reconnect...");
    wifi.connectBlocking();
    oled.printWrapped("WiFi OK");
  }
}