#include "io_devices.h"

namespace app {

IoDevices::IoDevices()
    : display_(config::display::kWidth, config::display::kHeight, &Wire, config::display::kReset) {}

void IoDevices::begin() {
  pinMode(config::pins::kLedRed, OUTPUT);
  pinMode(config::pins::kLedGreen, OUTPUT);
  pinMode(config::pins::kLedYellow, OUTPUT);
  pinMode(config::pins::kBuzzer, OUTPUT);

  const uint32_t now = millis();
  initButton(testButton_, config::pins::kButtonTest, now);
  initButton(startButton_, config::pins::kButtonStart, now);

  if (config::features::kEnableRelayOutput) {
    pinMode(config::pins::kRelay, OUTPUT);
  }

  analogReadResolution(12);
  analogSetPinAttenuation(config::pins::kMq3, ADC_11db);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  lockServo_.setPeriodHertz(config::servo::kPeriodHz);
  lockServo_.attach(config::pins::kServo, config::servo::kMinPulseUs, config::servo::kMaxPulseUs);

  forceSafeState();

  Wire.begin(config::pins::kOledSda, config::pins::kOledScl);
  displayReady_ = display_.begin(SSD1306_SWITCHCAPVCC, config::display::kI2cAddress);
  if (displayReady_) {
    display_.clearDisplay();
    display_.display();
  }
}

bool IoDevices::isDisplayReady() const {
  return displayReady_;
}

Adafruit_SSD1306& IoDevices::display() {
  return display_;
}

bool IoDevices::buttonPressed(ButtonId button, uint32_t nowMs) {
  ButtonTracker& tracker = buttonFor(button);
  const bool reading = digitalRead(tracker.pin);

  if (reading != tracker.lastRead) {
    tracker.lastRead = reading;
    tracker.lastChangeMs = nowMs;
  }

  if ((nowMs - tracker.lastChangeMs) > config::timing::kButtonDebounceMs && reading != tracker.lastStable) {
    tracker.lastStable = reading;
    return tracker.lastStable == tracker.activeLevel;
  }

  return false;
}

bool IoDevices::readAlcoholRaw(uint16_t& raw) {
  const int value = analogRead(config::pins::kMq3);
  if (value < config::thresholds::kAdcMin || value > config::thresholds::kAdcMax) {
    return false;
  }

  raw = static_cast<uint16_t>(value);
  return true;
}

void IoDevices::setBuzzer(bool on) {
  buzzerOn_ = on;
  digitalWrite(config::pins::kBuzzer, on ? HIGH : LOW);
}

void IoDevices::setIndicators(SystemState state) {
  clearLeds();

  switch (state) {
    case SystemState::Preheat:
    case SystemState::Sampling:
      digitalWrite(config::pins::kLedYellow, HIGH);
      break;

    case SystemState::PassReady:
    case SystemState::Running:
      digitalWrite(config::pins::kLedGreen, HIGH);
      break;

    case SystemState::FailLocked:
      digitalWrite(config::pins::kLedRed, HIGH);
      break;

    case SystemState::ErrorLocked:
      digitalWrite(config::pins::kLedRed, HIGH);
      digitalWrite(config::pins::kLedYellow, HIGH);
      break;

    case SystemState::StandbyLocked:
      break;
  }
}

void IoDevices::lockVehicle() {
  vehicleLocked_ = true;
  servoAngle_ = config::servo::kLockAngle;
  lockServo_.write(config::servo::kLockAngle);
  setRelay(false);
}

void IoDevices::unlockVehicle() {
  vehicleLocked_ = false;
  servoAngle_ = config::servo::kUnlockAngle;
  lockServo_.write(config::servo::kUnlockAngle);
  setRelay(true);
}

void IoDevices::forceSafeState() {
  setBuzzer(false);
  lockVehicle();
  clearLeds();
}

bool IoDevices::vehicleLocked() const {
  return vehicleLocked_;
}

bool IoDevices::buzzerOn() const {
  return buzzerOn_;
}

int IoDevices::servoAngle() const {
  return servoAngle_;
}

void IoDevices::initButton(ButtonTracker& button, int pin, uint32_t nowMs) {
  button.pin = pin;
  button.activeLevel = config::buttons::kActiveLevel;
  pinMode(pin, config::buttons::kPinMode);

  const bool reading = digitalRead(pin);
  button.lastStable = reading;
  button.lastRead = reading;
  button.lastChangeMs = nowMs;
}

IoDevices::ButtonTracker& IoDevices::buttonFor(ButtonId button) {
  return button == ButtonId::Test ? testButton_ : startButton_;
}

void IoDevices::clearLeds() {
  digitalWrite(config::pins::kLedRed, LOW);
  digitalWrite(config::pins::kLedGreen, LOW);
  digitalWrite(config::pins::kLedYellow, LOW);
}

void IoDevices::setRelay(bool enabled) {
  if (config::features::kEnableRelayOutput) {
    digitalWrite(config::pins::kRelay, enabled ? HIGH : LOW);
  }
}

}  // namespace app
