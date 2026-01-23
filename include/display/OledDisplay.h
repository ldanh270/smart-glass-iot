#pragma once
#include <Arduino.h>

class OledDisplay {
public:
  bool begin();
  void printWrapped(const String& text);

private:
  String safeTruncate(const String& s, size_t maxLen);
  String stripVietnamese(const String& in);
};
