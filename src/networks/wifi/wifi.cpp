#include "wifi.h"
#include "../../configs/networks.config.h"
#include "config.h"
#include <WiFi.h>

bool WiFiManager::connectBlocking() {
  // Chuyển WiFi về chế độ STA (station - mạch kết nối tới WiFi)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false); // Tắt kết nối cũ (không tắt WiFi)
  
  // Bắt đầu kết nối
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  unsigned long start_time = millis();
  
  // Block cho tới khi kết nối thành công hoặc timeout
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start_time > WIFI_TIMEOUT_MS) {
      // Timeout: kết nối thất bại
      WiFi.disconnect(false);
      return false;
    }
    delay(250);
  }
  
  return true; // Kết nối thành công
}

bool WiFiManager::isConnected() {
  // Trả true nếu mạch đang kết nối tới WiFi
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::disconnect() {
  // Ngắt kết nối WiFi
  WiFi.disconnect(true); // true = tắt WiFi radio
}
