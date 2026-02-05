#include <Arduino.h>
#include "drivers/display/display.h"
#include "networks/wifi/wifi.h"
#include <WiFi.h>

// Tạo đối tượng OledDisplay và WiFiManager
OledDisplay oled;
WiFiManager wifiManager;

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
        oled.print(0, 10, "IP: " + WiFi.localIP().toString());
        delay(3000);
    }
    else {
        Serial.println("WiFi connection failed!");
        oled.clear();
        oled.print(0, 0, "WiFi: Failed");
        delay(2000);
    }

    Serial.println("Setup completed!");
}

void loop() {
    // Kiểm tra trạng thái WiFi mỗi giây
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();

        oled.clear();

        // Kiểm tra trạng thái kết nối WiFi
        if (wifiManager.isConnected()) {
            // WiFi đã kết nối
            oled.print(0, 0, "WiFi: Connected");
            oled.print(0, 20, "IP: " + WiFi.localIP().toString());

            // Hiển thị thời gian uptime
            String timeStr = "Uptime: " + String(millis() / 1000) + "s";
            oled.print(0, 40, timeStr);

            Serial.println("WiFi Status: Connected");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
        }
        else {
            // WiFi đang kết nối hoặc mất kết nối
            oled.print(0, 0, "WiFi: Connecting...");

            String timeStr = "Time: " + String(millis() / 1000) + "s";
            oled.print(0, 60, timeStr);

            Serial.println("WiFi Status: Disconnected/Connecting...");
        }
    }
}