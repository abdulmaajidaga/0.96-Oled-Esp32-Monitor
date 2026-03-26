#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>
#include "config.h"

enum ButtonEvent {
    BTN_NONE = 0,
    BTN_UP_PRESSED,
    BTN_DOWN_PRESSED,
    BTN_ENTER_PRESSED,
    BTN_ENTER_LONG_PRESS
};

// Interrupt flags
volatile bool upFlag = false;
volatile bool downFlag = false;
volatile unsigned long upTime = 0;
volatile unsigned long downTime = 0;

void IRAM_ATTR onUpPress() {
    unsigned long now = millis();
    if (now - upTime > 200) {
        upFlag = true;
        upTime = now;
    }
}

void IRAM_ATTR onDownPress() {
    unsigned long now = millis();
    if (now - downTime > 200) {
        downFlag = true;
        downTime = now;
    }
}

class ButtonHandler {
private:
    bool lastEnter = true;
    unsigned long enterPressStart = 0;
    bool longPressTriggered = false;
    bool enterHandled = false;

public:
    void begin() {
        pinMode(BTN_UP_PIN, INPUT_PULLUP);
        pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
        pinMode(BTN_ENTER_PIN, INPUT_PULLUP);

        attachInterrupt(digitalPinToInterrupt(BTN_UP_PIN), onUpPress, FALLING);
        attachInterrupt(digitalPinToInterrupt(BTN_DOWN_PIN), onDownPress, FALLING);
    }

    ButtonEvent update() {
        unsigned long now = millis();

        // UP - instant from interrupt
        if (upFlag) {
            upFlag = false;
            return BTN_UP_PRESSED;
        }

        // DOWN - instant from interrupt
        if (downFlag) {
            downFlag = false;
            return BTN_DOWN_PRESSED;
        }

        // ENTER - manual polling with long press
        bool enterState = digitalRead(BTN_ENTER_PIN);

        if (enterState == LOW && lastEnter == HIGH) {
            // Just pressed
            enterPressStart = now;
            longPressTriggered = false;
            enterHandled = false;
        } else if (enterState == HIGH && lastEnter == LOW) {
            // Just released
            lastEnter = enterState;
            if (!longPressTriggered && !enterHandled) {
                return BTN_ENTER_PRESSED;
            }
            enterHandled = false;
            longPressTriggered = false;
        } else if (enterState == LOW && !longPressTriggered) {
            // Holding
            if (now - enterPressStart > 800) {
                longPressTriggered = true;
                enterHandled = true;
                return BTN_ENTER_LONG_PRESS;
            }
        }
        lastEnter = enterState;

        return BTN_NONE;
    }
};

#endif
