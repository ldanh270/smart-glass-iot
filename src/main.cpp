#include <Arduino.h>
#include "drivers/display/display.h"
#include "networks/wifi/wifi.h"
#include "drivers/button/button.h"
#include "drivers/mic/mic.h"
#include <WiFi.h>
#include <math.h>

// --- KHỞI TẠO ĐỐI TƯỢNG ---
OledDisplay oled;
WiFiManager wifiManager;
Button btnMode(2);
Button btnPower(3);
MicInmp441 mic;

// --- BIẾN TOÀN CỤC ---
volatile bool g_audioDetected = false;
volatile uint32_t g_lastAudioTime = 0;

// Callback Audio
void onAudioFrameReceived(const int16_t* pcm16, size_t samples) {
    g_audioDetected = true;
    g_lastAudioTime = millis();
    // Không tính toán nặng trong callback để tránh chặn CPU
}

void setup() {
    Serial.begin(115200);

    // 1. Init OLED
    if (!oled.init()) { Serial.println("OLED Failed"); while (1); }
    oled.print(0, 0, "System Booting...");
    oled.show(); // Đẩy hiển thị ra ngay lập tức

    // 2. Init WiFi
    oled.print(0, 10, "Connecting WiFi...");
    oled.show();

    if (wifiManager.connectBlocking()) {
        oled.print(0, 20, "WiFi OK!");
    }
    else {
        oled.print(0, 20, "WiFi Failed!");
    }
    oled.show();
    delay(1000);

    // 3. Init Hardware khác
    btnMode.init();
    btnPower.init();

    mic.init();
    mic.setThresholdRms(500.0f);
    mic.setOnAudioFrame(onAudioFrameReceived);
    mic.start();

    oled.clear();
    oled.show();
}

void loop() {
    // Process liên tục
    mic.process();
    // btnMode.update(); // Uncomment nếu thư viện nút cần

    // Xử lý logic nút bấm
    if (btnMode.isPressed()) {
        Serial.println("BTN1 Click");
    }

    if (btnPower.onHold()) {
        Serial.println("BTN2 Hold -> Toggle WiFi");
        if (wifiManager.isConnected()) wifiManager.disconnect();
        else wifiManager.connectBlocking();
    }

    // Logic Audio Timeout
    if (g_audioDetected && (millis() - g_lastAudioTime > 500)) {
        g_audioDetected = false;
    }

    // --- CẬP NHẬT MÀN HÌNH (50ms = 20 FPS) ---
    // 100ms hơi chậm, 50ms sẽ mượt hơn cho visualizer
    static uint32_t lastUi = 0;
    if (millis() - lastUi >= 50) {
        lastUi = millis();

        // 1. XÓA BUFFER (Chưa hiển thị gì cả)
        oled.clear();

        // 2. VẼ CÁC THÀNH PHẦN VÀO BUFFER

        // Ưu tiên hiển thị khi giữ nút
        if (btnMode.isDownRaw()) {
            oled.print(0, 20, "BTN1 HOLDING...");
            oled.show(); // Đẩy ra màn hình và kết thúc luôn frame này
            return;
        }

        // Tính toán Audio
        float rms = mic.lastRms();
        float dbfs = -90.0f;
        if (rms > 1.0f) {
            dbfs = 20.0f * log10f(rms / 32768.0f);
        }

        // Dòng 1: WiFi & Status
        String status = wifiManager.isConnected() ? "WiFi" : "Offline";
        if (g_audioDetected) status += " [REC]";
        oled.print(0, 0, status);

        // Dòng 2: IP hoặc Info
        if (wifiManager.isConnected()) {
            String ip = WiFi.localIP().toString();
            // Chỉ lấy đuôi IP cho gọn: ...1.105
            oled.print(0, 12, "IP:.." + ip.substring(ip.lastIndexOf('.')));
        }

        // Dòng 3: Số liệu Mic
        oled.print(0, 24, String(dbfs, 0) + "dB | RMS:" + String((int)rms));

        // Dòng 4: Thanh Bar
        int barWidth = constrain((int)(rms / 8000.0f * 14.0f), 0, 14);
        String bar = "[";
        for (int i = 0; i < 14; i++) bar += (i < barWidth) ? "=" : " ";
        bar += "]";
        oled.print(0, 36, bar);

        // Dòng 5: Uptime
        oled.print(0, 50, "Up: " + String(millis() / 1000) + "s");

        // 3. ĐẨY BUFFER RA MÀN HÌNH (QUAN TRỌNG NHẤT)
        // Đây là lúc duy nhất màn hình được refresh -> Hết chớp
        oled.show();
    }
}