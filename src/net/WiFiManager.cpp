#include "net/WiFiManager.h"
#include "config.h"

#include <WiFi.h>

void WiFiManager::connectBlocking() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
}

bool WiFiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}
