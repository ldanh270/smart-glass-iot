#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

/**
 * `Button`: quản lý nút nhấn vật lý
 * - init()        : khởi tạo button
 * - isPressed()   : nhấn ngắn (click)
 * - onHold()      : giữ lâu (hold)
 */

class Button {
private:
  uint8_t pin;

  bool lastState;
  bool currentState;

  unsigned long lastDebounceTime;
  unsigned long pressedTime;

  bool holdTriggered;

  static const unsigned long DEBOUNCE_MS = 50;
  static const unsigned long HOLD_TIME_MS = 1000; // giữ > 1s

public:
  // Khởi tạo button với chân GPIO
  Button(uint8_t pin);

  // Setup pinMode
  void init();

  // Trả true khi nhấn ngắn
  bool isPressed();

  // Trả true khi giữ lâu
  bool onHold();

  // Trả true nếu chân button đang ở trạng thái LOW (nhấn) - đọc thô, không debounce
  bool isDownRaw();
};

#endif
