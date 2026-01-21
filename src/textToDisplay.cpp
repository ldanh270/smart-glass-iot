/*******************************************************
 * MÀNG HÌNH KÍNH THÔNG MINH OUTPUT (ESP32-S3)
 * - Nhận TEXT đã dịch qua WebSocket (JSON)
 * - Hiển thị TEXT lên màn hình OLED SSD1306 (I2C)
 * - Phát giọng nói bằng TTS phía server:
 *     ESP32 -> HTTP POST /tts  (gửi text)
 *     Server -> trả về PCM16 mono 16kHz (raw) HOẶC WAV
 *     ESP32 phát qua I2S tới MAX98357A -> loa
 *
 * Phần cứng bạn có:
 * - ESP32-S3 WROOM N16R8
 * - OLED 0.96" SSD1306 I2C
 * - Mạch khuếch đại I2S MAX98357A + loa 8ohm 1W
 *
 * Ghi chú:
 * - Code này CHƯA dùng mic/camera (chỉ xử lý OUTPUT).
 * - Với tiếng Việt trên OLED: font mặc định không hỗ trợ UTF-8 tốt.
 *   -> Có tuỳ chọn “loại bỏ dấu tiếng Việt” để dễ hiển thị.
 *******************************************************/

#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ESP32 I2S low-level driver
#include "driver/i2s.h"

using namespace websockets;

/************ CẤU HÌNH NGƯỜI DÙNG ************/
// WiFi
const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASS = "your_pass";

// URL Server
// WebSocket: server gửi kết quả dịch dạng JSON tại đây
const char* WS_URL  = "ws://192.168.1.10:8080/ws";

// Điểm cuối TTS: ESP32 gửi text ở đây, server trả về audio
// Kỳ vọng ví dụ: POST http://192.168.1.10:8080/tts
const char* TTS_URL = "http://192.168.1.10:8080/tts";

// Định dạng audio được server trả về (THỰC HÀNH TỐT):
// - PCM 16-bit có dấu little-endian, mono, 16000 Hz (raw)
// HOẶC WAV tiêu chuẩn bao quanh PCM đó (chúng ta tự động bỏ qua WAV header).

// Cấu hình OLED
#define OLED_ADDR 0x3C
#define OLED_W    128
#define OLED_H    64

// Chọn nếu bạn muốn loại bỏ dấu tiếng Việt để OLED dễ đọc hơn
#define STRIP_VIETNAMESE_FOR_OLED 1

/************ CẤU HÌNH CHÂN (CHỈNH SỬA THEO CÂY NỐI CỦA BẠN) ************/
// Chân I2C OLED (ESP32-S3 có thể ánh xạ I2C tới nhiều chân; đặt chân của bạn)
#define I2C_SDA 8
#define I2C_SCL 9

// Chân I2S MAX98357A (đặt dựa trên cây nối của bạn)
#define I2S_BCLK  5   // BCLK (Bit Clock)
#define I2S_LRCLK 6   // LRC / WS (Word Select)
#define I2S_DOUT  4   // DIN (Data vào MAX98357A)

/************ BIẾN TOÀN CỤC ************/
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
WebsocketsClient ws;

// Quản lý kết nối
static unsigned long lastReconnectAttemptMs = 0;
static const unsigned long RECONNECT_INTERVAL_MS = 2000;

// Trạng thái đơn giản để ngăn ngừa giọng nói chồng chéo
volatile bool isSpeaking = false;

// Cấu hình I2S
static const int AUDIO_SAMPLE_RATE = 16000;
static const int AUDIO_BITS = 16;
static const int AUDIO_CHANNELS = 1; // mono

/************ CÁC TIỆN ÍCH ************/
static String safeTruncate(const String& s, size_t maxLen) {
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen);
}

// Hàm loại bỏ dấu rất nhỏ (hiển thị tiếng Việt trên font OLED mặc định bị hạn chế)
// Ghi chú: Đây là cách tiếp cận đơn giản; để hiển thị tiếng Việt hoàn hảo, hãy dùng GFXfont tùy chỉnh.
static String stripVietnamese(const String& in) {
#if STRIP_VIETNAMESE_FOR_OLED
  String out; out.reserve(in.length());
  // Chuyển đổi ký tự tiếng Việt UTF-8 thành xấp xỉ ASCII (ánh xạ không hoàn chỉnh)
  // Ánh xạ này không toàn diện nhưng giúp khả năng đọc nhanh chóng.
  for (size_t i = 0; i < in.length(); i++) {
    unsigned char c = (unsigned char)in[i];
    if (c < 128) { out += (char)c; continue; }

    // ngây thơ: bỏ qua UTF-8 đa byte và thay thế bằng '?'
    // Tốt hơn: triển khai bản đồ Unicode đầy đủ; ở đây giữ đơn giản để ổn định.
    out += '?';
  }
  return out;
#else
  return in;
#endif
}

// Ngắt dòng theo khoảng trắng (dễ đọc hơn cắt theo số ký tự)
static void oledPrintWrapped(const String& text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const int lineHeight = 10;
  const int maxLines = OLED_H / lineHeight;  // ~6 dòng
  const int maxCharsPerLine = 21;            // xấp xỉ cho font 5x7 mặc định ở kích thước 1

  String t = stripVietnamese(text);
  t.trim();

  int y = 0;
  int lineCount = 0;

  int start = 0;
  while (start < (int)t.length() && lineCount < maxLines) {
    // Nếu phần còn lại vừa một dòng
    int remaining = (int)t.length() - start;
    int take = min(remaining, maxCharsPerLine);

    // Cố gắng ngắt tại khoảng trắng cuối cùng trong [start, start+take)
    int end = start + take;
    int breakPos = -1;

    for (int i = end; i > start; i--) {
      if (t[i - 1] == ' ') { breakPos = i - 1; break; }
    }

    if (breakPos == -1 || remaining <= maxCharsPerLine) {
      // Không tìm thấy khoảng trắng hoặc dòng cuối cùng
      breakPos = min(start + maxCharsPerLine, (int)t.length());
    }

    String line = t.substring(start, breakPos);
    line.trim();

    display.setCursor(0, y);
    display.print(line);

    y += lineHeight;
    lineCount++;

    // Di chuyển start tiến lên (bỏ qua khoảng trắng)
    start = breakPos;
    while (start < (int)t.length() && t[start] == ' ') start++;
  }

  display.display();
}

/************ OUTPUT AUDIO I2S ************/
static bool initI2S() {
  // LƯU Ý: MAX98357A kỳ vọng chuẩn I2S, dữ liệu trên DIN, cần BCLK + LRCLK + DIN.
  i2s_config_t i2s_config = {};
  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2s_config.sample_rate = AUDIO_SAMPLE_RATE;
  i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT; // mono -> trái
  i2s_config.communication_format = I2S_COMM_FORMAT_I2S; // I2S chuẩn
  i2s_config.intr_alloc_flags = 0;
  i2s_config.dma_buf_count = 8;        // LƯU Ý: tăng nếu âm thanh bị giật
  i2s_config.dma_buf_len = 256;        // LƯU Ý: kích thước khối; điều chỉnh để ổn định
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = true;
  i2s_config.fixed_mclk = 0;

  // Cài đặt trình điều khiển
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) return false;

  // Ánh xạ chân
  i2s_pin_config_t pin_config = {};
  pin_config.bck_io_num = I2S_BCLK;
  pin_config.ws_io_num  = I2S_LRCLK;
  pin_config.data_out_num = I2S_DOUT;
  pin_config.data_in_num  = I2S_PIN_NO_CHANGE;

  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) return false;

  // Đảm bảo xung nhịp chính xác
  err = i2s_set_clk(I2S_NUM_0, AUDIO_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  return (err == ESP_OK);
}

static void stopAudio() {
  // LƯU Ý: xả DMA để dừng phát hiện tại sạch sẽ
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// Phát hiện WAV header "RIFF" và bỏ qua 44 byte đầu tiên
static bool looksLikeWavHeader(const uint8_t* buf, size_t len) {
  if (len < 12) return false;
  return (buf[0]=='R' && buf[1]=='I' && buf[2]=='F' && buf[3]=='F');
}

/************ YÊU CẦU TTS + PHÁT ************/
// Hợp đồng server (được khuyến nghị):
// POST /tts với JSON: {"text":"...", "lang":"vi"}
// Phản hồi: audio/wav HOẶC application/octet-stream (PCM16 16k mono)
// Chúng ta phát trực tuyến phản hồi tới I2S theo từng khối để tránh sử dụng RAM.
static bool requestTTSAndPlay(const String& text, const String& lang) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(TTS_URL);
  http.addHeader("Content-Type", "application/json");

  // LƯU Ý: giữ payload nhỏ; nếu text quá dài, cắt ngắn để an toàn
  String payload;
  payload.reserve(256);
  payload = "{\"text\":\"";
  // Escape JSON cơ bản cho dấu ngoặc/dấu gạch chéo (tối thiểu)
  for (size_t i = 0; i < text.length(); i++) {
    char ch = text[i];
    if (ch == '\\' || ch == '"') payload += '\\';
    payload += ch;
  }
  payload += "\",\"lang\":\"";
  payload += lang;
  payload += "\"}";

  int code = http.POST(payload);
  if (code <= 0) {
    http.end();
    return false;
  }
  if (code != 200) {
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();

  isSpeaking = true;

  // LƯU Ý: Chúng ta phát trực tuyến âm thanh tới I2S, từng khối một.
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
      // Ngắt nếu không có dữ liệu quá lâu (mạng bị treo)
      if (millis() - lastDataMs > 5000) break;
      continue;
    }

    int toRead = min(avail, (int)BUF_SZ);
    int n = stream->readBytes(buf, toRead);
    if (n <= 0) continue;

    lastDataMs = millis();

    // Kiểm tra WAV header một lần (nếu được trả về dạng WAV)
    if (!headerChecked) {
      headerChecked = true;
      if (looksLikeWavHeader(buf, n)) {
        skippedWavHeader = true;
      }
    }

    size_t offset = 0;
    if (skippedWavHeader && wavSkipLeft > 0) {
      size_t skipNow = min((size_t)n, wavSkipLeft);
      offset += skipNow;
      wavSkipLeft -= skipNow;
      if (offset >= (size_t)n) continue; // tất cả bỏ qua
    }

    // Ghi PCM còn lại tới I2S
    size_t bytesWritten = 0;
    esp_err_t err = i2s_write(I2S_NUM_0, buf + offset, n - offset, &bytesWritten, portMAX_DELAY);
    if (err != ESP_OK) break;
  }

  stopAudio();
  isSpeaking = false;

  http.end();
  return true;
}

/************ WIFI + WS ************/
static void connectWiFiBlocking() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  oledPrintWrapped("WiFi connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  oledPrintWrapped("WiFi OK");
}

// Kết nối WS mạnh mẽ với các callback
static bool connectWebSocket() {
  ws.onMessage([&](WebsocketsMessage msg) {
    // LƯU Ý: Kỳ vọng JSON như:
    // {"type":"translate_result","text":"...","lang":"vi"}
    StaticJsonDocument<1024> doc; // LƯU Ý: lớn hơn cho text dài hơn
    DeserializationError err = deserializeJson(doc, msg.data());
    if (err) return;

    const char* type = doc["type"] | "";
    if (String(type) != "translate_result") return;

    String text = doc["text"] | "";
    String lang = doc["lang"] | "vi";

    // LƯU Ý: Hiển thị ngay lập tức (phản hồi UI nhanh)
    oledPrintWrapped(safeTruncate(text, 300));

    // LƯU Ý: Phát nó (TTS phía server -> audio -> I2S)
    // Tránh giọng nói chồng chéo nếu kết quả tới nhanh chóng
    if (!isSpeaking) {
      // Tuỳ chọn: hiển thị trạng thái trên OLED
      // oledPrintWrapped("Speaking...\n" + safeTruncate(text, 120));
      requestTTSAndPlay(safeTruncate(text, 300), lang);
    }
  });

  ws.onEvent([&](WebsocketsEvent e, String data) {
    if (e == WebsocketsEvent::ConnectionOpened) {
      oledPrintWrapped("WS Connected");
    } else if (e == WebsocketsEvent::ConnectionClosed) {
      oledPrintWrapped("WS Closed\nReconnecting...");
    }
  });

  // LƯU Ý: connect() trả về bool trong ArduinoWebsockets
  bool ok = ws.connect(WS_URL);
  return ok;
}

static void ensureConnections() {
  // Kết nối lại WiFi
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    connectWiFiBlocking();
  }

  // WS kết nối lại với giới hạn
  if (!ws.available()) { // LƯU Ý: ws.available() chỉ ra kết nối còn sống
    unsigned long now = millis();
    if (now - lastReconnectAttemptMs > RECONNECT_INTERVAL_MS) {
      lastReconnectAttemptMs = now;
      connectWebSocket();
    }
  }
}

/************ SETUP / LOOP ************/
void setup() {
  Serial.begin(115200);

  // Khởi tạo I2C cho OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // LƯU Ý: Nếu khởi tạo OLED thất bại, dừng tại đây (lỗi cứng)
    while (true) { delay(1000); }
  }
  oledPrintWrapped("Booting...");

  // Khởi tạo I2S cho MAX98357A
  if (!initI2S()) {
    oledPrintWrapped("I2S init FAIL");
    while (true) { delay(1000); }
  }

  connectWiFiBlocking();

  bool wsOk = connectWebSocket();
  if (!wsOk) {
    oledPrintWrapped("WS connect FAIL\nWill retry...");
  }
}

void loop() {
  // Giữ WS sống
  ws.poll();

  // Kết nối lại tự động nếu cần (WiFi + WS)
  ensureConnections();
}
