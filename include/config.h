#pragma once

/************ CẤU HÌNH NGƯỜI DÙNG ************/
// WiFi - Thay "your_ssid" và "your_pass" bằng tên mạng và password của bạn
static const char* WIFI_SSID = "Daily Coffee";     // Tên WiFi (SSID)
static const char* WIFI_PASS = "666666666";     // Mật khẩu WiFi

// URL Server
static const char* WS_URL  = "http://localhost:5000";
static const char* TTS_URL = "https://smart-glass-server.vercel.app/tts";

// OLED config
#define OLED_ADDR 0x3C
#define OLED_W    128
#define OLED_H    64

// Chọn nếu bạn muốn loại bỏ dấu tiếng Việt để OLED dễ đọc hơn
#define STRIP_VIETNAMESE_FOR_OLED 1

/************ CẤU HÌNH CHÂN (CHỈNH SỬA THEO CÂY NỐI ) ************/
// Chân I2C OLED
#define I2C_SDA 19
#define I2C_SCL 20

// Chân I2S MAX98357A
#define I2S_BCLK  38   // BCLK (Bit Clock)
#define I2S_LRCLK 39   // LRC / WS (Word Select)
#define I2S_DOUT  40   // DIN (Data vào MAX98357A)

// Audio format
static const int AUDIO_SAMPLE_RATE = 16000;
static const int AUDIO_BITS = 16;
static const int AUDIO_CHANNELS = 1; // mono

// Ghi chú cấu hình:
// - Thay `WIFI_SSID` / `WIFI_PASS` bằng thông tin mạng.
// - `WS_URL` / `TTS_URL` là endpoint server; đảm bảo server hỗ trợ WebSocket/TTS.
// - Chỉnh chân I2C/I2S tuỳ theo board và mạch.
// - `STRIP_VIETNAMESE_FOR_OLED`: bật (=1) nếu OLED không hiển thị UTF-8 tốt.
