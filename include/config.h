#pragma once

/************ CẤU HÌNH NGƯỜI DÙNG ************/
// WiFi
static const char* WIFI_SSID = "your_ssid";
static const char* WIFI_PASS = "your_pass";

// URL Server
static const char* WS_URL  = "ws://192.168.1.10:8080/ws";
static const char* TTS_URL = "http://192.168.1.10:8080/tts";

// OLED config
#define OLED_ADDR 0x3C
#define OLED_W    128
#define OLED_H    64

// Chọn nếu bạn muốn loại bỏ dấu tiếng Việt để OLED dễ đọc hơn
#define STRIP_VIETNAMESE_FOR_OLED 1

/************ CẤU HÌNH CHÂN (CHỈNH SỬA THEO CÂY NỐI CỦA BẠN) ************/
// Chân I2C OLED
#define I2C_SDA 8
#define I2C_SCL 9

// Chân I2S MAX98357A
#define I2S_BCLK  5   // BCLK (Bit Clock)
#define I2S_LRCLK 6   // LRC / WS (Word Select)
#define I2S_DOUT  4   // DIN (Data vào MAX98357A)

// Audio format
static const int AUDIO_SAMPLE_RATE = 16000;
static const int AUDIO_BITS = 16;
static const int AUDIO_CHANNELS = 1; // mono
