#include <Arduino.h>
#include "drivers/display/display.h"
#include "networks/wifi/wifi.h"
#include "drivers/button/button.h"
#include "drivers/mic/mic.h"
#include <WiFi.h>
#include <math.h>

// Tạo đối tượng OledDisplay và WiFiManager
OledDisplay oled;
WiFiManager wifiManager;

Button btnMode(2);   // BTN1
Button btnPower(3);  // BTN2

MicInmp441 mic;

// Biến để lưu trạng thái audio
volatile bool g_audioDetected = false;
volatile float g_currentRms = 0.0f;
volatile uint32_t g_lastAudioTime = 0;

// Callback khi có audio frame
void onAudioFrameReceived(const int16_t* pcm16, size_t samples) {
    g_audioDetected = true;
    g_lastAudioTime = millis();

    // Tính RMS từ pcm16
    int64_t sumSq = 0;
    for (size_t i = 0; i < samples; i++) {
        sumSq += (int64_t)pcm16[i] * (int64_t)pcm16[i];
    }
    g_currentRms = sqrtf((float)sumSq / (float)samples);

    Serial.printf("[AUDIO] Detected! RMS=%.1f, samples=%u\n", g_currentRms, samples);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Starting OLED Display and WiFi Test...");

    // Test 1: Khởi tạo OLED
    if (oled.init()) {
        Serial.println("OLED initialized successfully!");
    }
    else {
        Serial.println("OLED initialization failed!");
        while (1); // Dừng nếu không khởi tạo được
    }

    delay(1000);

    // Test 2: Kết nối WiFi
    Serial.println("Connecting to WiFi...");
    oled.clear();
    oled.print(0, 0, "WiFi: Connecting...");

    if (wifiManager.connectBlocking()) {
        Serial.println("WiFi connected successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        oled.clear();
        oled.print(0, 0, "WiFi: Connected");
        // oled.print(0, 10, "IP: " + WiFi.localIP().toString());
        delay(3000);
    }
    else {
        Serial.println("WiFi connection failed!");
        oled.clear();
        oled.print(0, 0, "WiFi: Failed");
        delay(2000);
    }

    Serial.println("Setup completed!");

    // Khởi tạo các nút bấm
    btnMode.init();
    btnPower.init();

    mic.init();

    // Đặt ngưỡng RMS threshold (điều chỉnh giá trị này phù hợp với mic của bạn)
    mic.setThresholdRms(500.0f);  // Bắt đầu từ 500, có thể tăng/giảm

    // Đăng ký callback
    mic.setOnAudioFrame(onAudioFrameReceived);

    mic.start();
    Serial.println("Button initialized!");
}

void loop() {
    // đọc nút (tùy Button lib của bạn có cần update() hay không)
    // btnMode.update();
    // btnPower.update();

    // luôn đọc mic
    mic.process();

    // BTN1
    if (btnMode.isPressed()) {
        Serial.println("[BTN1] Pressed → Switch mode");
    }
    if (btnMode.onHold()) {
        Serial.println("[BTN1] Hold → long press action");
    }

    // BTN2 hold: toggle wifi
    if (btnPower.onHold()) {
        Serial.println("[BTN2] Hold → Toggle WiFi");
        if (wifiManager.isConnected()) {
            wifiManager.disconnect();
        }
        else {
            wifiManager.connectBlocking();
        }
    }

    // Kiểm tra timeout audio (nếu quá 500ms không có audio thì tắt flag)
    if (g_audioDetected && (millis() - g_lastAudioTime > 500)) {
        g_audioDetected = false;
    }

    // UI realtime (10Hz)
    static uint32_t lastUi = 0;
    if (millis() - lastUi >= 100) {
        lastUi = millis();

        // nếu đang giữ BTN1 thì cho ưu tiên màn hình riêng, không overwrite
        if (btnMode.isDownRaw()) {
            oled.clear();
            oled.print(0, 0, "BTN1 Hold");
            return;
        }

        float rms = mic.lastRms();
        float dbfs = -90.0f;
        if (rms > 1.0f) {
            dbfs = 20.0f * log10f(rms / 32768.0f);
            if (dbfs < -90.0f) dbfs = -90.0f;
        }

        // Vẽ 1 frame: clear 1 lần thôi
        oled.clear();

        // dòng trạng thái
        String statusLine = wifiManager.isConnected() ? "WiFi: OK" : "WiFi: ...";
        if (g_audioDetected) {
            statusLine += " [REC]";  // Hiển thị đang ghi âm
        }
        oled.print(0, 0, statusLine);

        // IP (nếu muốn)
        if (wifiManager.isConnected()) {
            oled.print(0, 10, "IP: " + WiFi.localIP().toString());
        }

        // mic info
        oled.print(0, 24, "dBFS: " + String(dbfs, 1));
        oled.print(0, 36, "RMS : " + String(rms, 1));

        // Vẽ thanh âm lượng (volume bar)
        // Map RMS từ 0-10000 xuống 0-100 pixels
        int barWidth = constrain((int)(rms / 10000.0f * 100.0f), 0, 100);
        String bar = "[";
        int numBars = barWidth / 10;  // Mỗi ký tự đại diện 10 pixels
        for (int i = 0; i < 10; i++) {
            bar += (i < numBars) ? "=" : " ";
        }
        bar += "]";
        oled.print(0, 48, bar);

        // uptime
        oled.print(0, 58, "Up: " + String(millis() / 1000) + "s");
    }

    // bỏ dòng này vì spam cực mạnh -> giật + chớp
    // Serial.println("Loop completed!");
}