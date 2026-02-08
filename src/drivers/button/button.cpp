#include "button.h"

Button::Button(uint8_t pin)
    : pin(pin),
    lastState(HIGH),
    currentState(HIGH),
    lastDebounceTime(0),
    pressedTime(0),
    holdTriggered(false) {
}

void Button::init() {
    pinMode(pin, INPUT);
}

bool Button::isPressed() {
    bool reading = digitalRead(pin);

    if (reading != lastState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
        if (reading != currentState) {
            currentState = reading;

            // Khi thả nút ra
            if (currentState == HIGH && !holdTriggered) {
                lastState = reading;
                return true; // nhấn ngắn
            }

            // Khi nhấn nút xuống
            if (currentState == LOW) {
                pressedTime = millis();
                holdTriggered = false;
            }
        }
    }

    lastState = reading;
    return false;
}

bool Button::onHold() {
    if (currentState == LOW && !holdTriggered) {
        if (millis() - pressedTime >= HOLD_TIME_MS) {
            holdTriggered = true;
            return true;
        }
    }
    return false;
}

bool Button::isDownRaw() {
    return digitalRead(pin) == LOW;
}
