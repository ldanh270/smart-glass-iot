#pragma once
#include <Arduino.h>

class WiFiManager {
public:
  void connectBlocking();
  bool isConnected() const;
};
