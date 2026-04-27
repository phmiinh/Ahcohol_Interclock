#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <Wire.h>

#include "app_types.h"

namespace app {

enum class ButtonId : uint8_t {
  Test,
  Start
};

class IoDevices {
 public:
  IoDevices();

  void begin();
  bool isDisplayReady() const;
  Adafruit_SSD1306& display();
  bool buttonPressed(ButtonId button, uint32_t nowMs);
  bool buttonActive(ButtonId button);
  void clearButtonEvent(ButtonId button, uint32_t nowMs);
  uint16_t readAlcoholRaw();
  void setBuzzer(bool on);
  void setIndicators(SystemState state);
  void lockVehicle();
  void unlockVehicle();
  void forceSafeState();
  bool vehicleLocked() const;
  bool buzzerOn() const;
  int servoAngle() const;

 private:
  // Keep the buzzer on an LEDC timer that is not shared with the servo setup.
  static constexpr uint8_t kBuzzerLedcChannel = 6;
  static constexpr uint32_t kBuzzerFreqHz = 2000;
  static constexpr uint8_t kBuzzerLedcResolution = 8;

  struct ButtonTracker {
    int pin = -1;
    bool activeLevel = config::buttons::kActiveLevel;
    bool lastStable = config::buttons::kIdleLevel;
    bool lastRead = config::buttons::kIdleLevel;
    uint32_t lastChangeMs = 0;
    uint32_t lastPressEventMs = 0;
  };

  static void IRAM_ATTR onTestButtonIsr();
  static void IRAM_ATTR onStartButtonIsr();
  static volatile bool testIrqFlag_;
  static volatile bool startIrqFlag_;

  void initButton(ButtonTracker& button, int pin, uint32_t nowMs);
  bool consumeButtonIrq(ButtonId button);
  ButtonTracker& buttonFor(ButtonId button);
  void clearLeds();
  void setRelay(bool enabled);

  Adafruit_SSD1306 display_;
  Servo lockServo_;
  ButtonTracker testButton_;
  ButtonTracker startButton_;
  bool displayReady_ = false;
  bool vehicleLocked_ = true;
  bool buzzerOn_ = false;
  int servoAngle_ = config::servo::kLockAngle;
};

}  // namespace app
