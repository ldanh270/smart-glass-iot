#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>

// `WiFiManager`: wrapper quản lý kết nối WiFi cho mạch IoT.
// - `connectBlocking(timeout_ms)` sẽ cố kết nối tới WiFi và block cho tới khi
//   kết nối thành công hoặc timeout.
// - `isConnected()` trả về trạng thái kết nối hiện tại.
// - `disconnect()` ngắt kết nối WiFi.
class WiFiManager {
public:
  // Kết nối tới WiFi với timeout (mili giây). Mặc định 30 giây.
  bool connectBlocking(unsigned long timeout_ms = 30000);
  
  // Kiểm tra trạng thái kết nối
  bool isConnected();
  
  // Ngắt kết nối WiFi
  void disconnect();
};

#endif
