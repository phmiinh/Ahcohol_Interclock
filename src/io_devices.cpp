#include "io_devices.h"

namespace app {

volatile bool IoDevices::testIrqFlag_ = false;
volatile bool IoDevices::startIrqFlag_ = false;

void IRAM_ATTR IoDevices::onTestButtonIsr() {
  testIrqFlag_ = true;
}

void IRAM_ATTR IoDevices::onStartButtonIsr() {
  startIrqFlag_ = true;
}

IoDevices::IoDevices()
    : display_(config::display::kWidth, config::display::kHeight, &Wire, config::display::kReset) {}

void IoDevices::begin() {
  pinMode(config::pins::kLedRed, OUTPUT);
  pinMode(config::pins::kLedGreen, OUTPUT);
  pinMode(config::pins::kLedYellow, OUTPUT);
  pinMode(config::pins::kBuzzer, OUTPUT);
  ledcSetup(kBuzzerLedcChannel, kBuzzerFreqHz, kBuzzerLedcResolution);
  ledcAttachPin(config::pins::kBuzzer, kBuzzerLedcChannel);
  ledcWrite(kBuzzerLedcChannel, 0);

  const uint32_t now = millis();
  initButton(testButton_, config::pins::kButtonTest, now);
  attachInterrupt(
      digitalPinToInterrupt(config::pins::kButtonTest),
      IoDevices::onTestButtonIsr,
      config::buttons::kActiveHigh ? RISING : FALLING);
  initButton(startButton_, config::pins::kButtonStart, now);
  attachInterrupt(
      digitalPinToInterrupt(config::pins::kButtonStart),
      IoDevices::onStartButtonIsr,
      config::buttons::kActiveHigh ? RISING : FALLING);

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

  if (consumeButtonIrq(button) && (nowMs - tracker.lastPressEventMs) > config::timing::kButtonDebounceMs) {
    tracker.lastPressEventMs = nowMs;
    tracker.lastStable = tracker.activeLevel;
    tracker.lastRead = reading;
    tracker.lastChangeMs = nowMs;
    return true;
  }

  if (reading != tracker.lastRead) {
    tracker.lastRead = reading;
    tracker.lastChangeMs = nowMs;
  }

  if ((nowMs - tracker.lastChangeMs) > config::timing::kButtonDebounceMs && reading != tracker.lastStable) {
    tracker.lastStable = reading;
    if (tracker.lastStable == tracker.activeLevel &&
        (nowMs - tracker.lastPressEventMs) > config::timing::kButtonDebounceMs) {
      tracker.lastPressEventMs = nowMs;
      return true;
    }
  }

  return false;
}

bool IoDevices::buttonActive(ButtonId button) {
  const ButtonTracker& tracker = buttonFor(button);
  return digitalRead(tracker.pin) == tracker.activeLevel;
}

uint16_t IoDevices::readAlcoholRaw() {
  const int value = analogRead(config::pins::kMq3);
  const int clamped = constrain(value, config::thresholds::kAdcMin, config::thresholds::kAdcMax);
  return static_cast<uint16_t>(clamped);
}

void IoDevices::setBuzzer(bool on) {
  buzzerOn_ = on;
  ledcWrite(kBuzzerLedcChannel, on ? 128 : 0);
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

    case SystemState::RetestRequired:
    case SystemState::RetestSampling:
      digitalWrite(config::pins::kLedGreen, HIGH);
      digitalWrite(config::pins::kLedYellow, HIGH);
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
  button.lastPressEventMs = 0;
}

bool IoDevices::consumeButtonIrq(ButtonId button) {
  if (button == ButtonId::Test && testIrqFlag_) {
    testIrqFlag_ = false;
    return true;
  }

  if (button == ButtonId::Start && startIrqFlag_) {
    startIrqFlag_ = false;
    return true;
  }

  return false;
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
