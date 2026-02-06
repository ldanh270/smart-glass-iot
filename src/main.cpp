#include <Arduino.h>
#include "drivers/display/display.h"
#include "networks/wifi/wifi.h"
#include "drivers/button/button.h"
#include <WiFi.h>

// Tạo đối tượng OledDisplay và WiFiManager
OledDisplay oled;
WiFiManager wifiManager;

Button btnMode(2);   // BTN1
Button btnPower(3);  // BTN2

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

    // Khởi tạo các nút bấm
    btnMode.init();
    btnPower.init();
    
    Serial.println("Button initialized!");
}

void loop() {
    // Kiểm tra nút bấm Mode
    if (btnMode.isPressed()) {
        Serial.println("[BTN1] Pressed → Switch mode");
        oled.clear();
        oled.print(0, 0, "BTN1 Pressed");
    }

    // Kiểm tra giữ nút Mode (bấm giữ)
    if (btnMode.onHold()) {
        Serial.println("[BTN1] Hold → long press action");
        oled.clear();
        oled.print(0, 0, "BTN1 Hold");
        // TODO: Thêm hành động khi giữ nút (ví dụ: chuyển chế độ dài, reset, v.v.)
    }

    if (btnPower.onHold()) {
        Serial.println("[BTN2] Hold → Toggle WiFi");

        if (wifiManager.isConnected()) {
            wifiManager.disconnect();
            oled.clear();
            oled.print(0, 0, "WiFi: OFF");
        } else {
            oled.clear();
            oled.print(0, 0, "WiFi: Reconnecting");
            wifiManager.connectBlocking();
        }
    }

    // Kiểm tra trạng thái WiFi mỗi giây
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();

        // Nếu đang giữ nút Mode thì không ghi đè lên màn hình (giữ hiển thị "BTN1 Hold")
        if (!btnMode.isDownRaw()) {
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
}