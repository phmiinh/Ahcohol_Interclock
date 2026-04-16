# 1 "C:\\Users\\Admin\\AppData\\Local\\Temp\\tmpkbi7qm64"
#include <Arduino.h>
# 1 "D:/PTIT/Embedded System alcohol_interlock/sketch.ino"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
# 16 "D:/PTIT/Embedded System alcohol_interlock/sketch.ino"
static const int PIN_MQ3 = 34;
static const int PIN_SERVO = 15;
static const int PIN_BUZZER = 13;
static const int PIN_LED_RED = 25;
static const int PIN_LED_GREEN = 26;
static const int PIN_LED_YELLOW = 27;
static const int PIN_BTN_TEST = 14;
static const int PIN_BTN_START = 12;


static const bool ENABLE_RELAY_OUTPUT = false;
static const int PIN_RELAY = 33;


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


Servo lockServo;
static const int SERVO_LOCK_ANGLE = 0;
static const int SERVO_UNLOCK_ANGLE = 90;


static const bool DEMO_MODE = true;
static const uint32_t PREHEAT_MS = DEMO_MODE ? 10000 : 60000;
static const uint16_t ALCOHOL_THRESHOLD = 2000;
static const uint8_t SAMPLE_COUNT = 20;
static const uint32_t SAMPLE_TOTAL_MS = 1000;
static const uint32_t BUTTON_DEBOUNCE_MS = 60;
static const uint32_t UI_REFRESH_MS = 200;
static const uint32_t FAIL_BUZZER_PERIOD = 500;
static const uint32_t FAIL_BUZZER_ON_MS = 220;


enum SystemState {
  STATE_PREHEAT,
  STATE_STANDBY_LOCKED,
  STATE_SAMPLING,
  STATE_PASS_READY,
  STATE_FAIL_LOCKED,
  STATE_RUNNING
};

SystemState state = STATE_PREHEAT;
uint32_t preheatStartMs = 0;
uint32_t lastUiRefreshMs = 0;
uint32_t lastFailBuzzerToggleMs = 0;

uint16_t lastAlcoholRaw = 0;
uint16_t sampledAlcoholRaw = 0;
bool buzzerFailState = false;


struct ButtonTracker {
  int pin;
  bool lastStable;
  bool lastRead;
  uint32_t lastChangeMs;

  ButtonTracker(int buttonPin, bool stableState = HIGH, bool readState = HIGH, uint32_t changedAtMs = 0)
    : pin(buttonPin), lastStable(stableState), lastRead(readState), lastChangeMs(changedAtMs) {}
};

ButtonTracker btnTest(PIN_BTN_TEST);
ButtonTracker btnStart(PIN_BTN_START);
void setRelay(bool enabled);
void setBuzzer(bool on);
void lockVehicle();
void unlockVehicle();
uint16_t readAlcoholRaw();
float rawToPercent(uint16_t raw);
void clearLeds();
void applyIndicators();
void drawStatusScreen();
bool buttonPressed(ButtonTracker &btn);
uint16_t sampleAlcoholAverage();
void transitionTo(SystemState newState);
void handlePreheat();
void handleSampling();
void handleFailBuzzer();
void setup();
void loop();
#line 85 "D:/PTIT/Embedded System alcohol_interlock/sketch.ino"
void setRelay(bool enabled) {
  if (ENABLE_RELAY_OUTPUT) {
    digitalWrite(PIN_RELAY, enabled ? HIGH : LOW);
  }
}

void setBuzzer(bool on) {
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

void lockVehicle() {
  lockServo.write(SERVO_LOCK_ANGLE);
  setRelay(false);
}

void unlockVehicle() {
  lockServo.write(SERVO_UNLOCK_ANGLE);
  setRelay(true);
}

uint16_t readAlcoholRaw() {

  int raw = analogRead(PIN_MQ3);
  if (raw < 0) raw = 0;
  if (raw > 4095) raw = 4095;
  return (uint16_t)raw;
}

float rawToPercent(uint16_t raw) {
  return (raw / 4095.0f) * 100.0f;
}

void clearLeds() {
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
}

void applyIndicators() {
  clearLeds();

  switch (state) {
    case STATE_PREHEAT:
      digitalWrite(PIN_LED_YELLOW, HIGH);
      break;

    case STATE_STANDBY_LOCKED:

      break;

    case STATE_SAMPLING:
      digitalWrite(PIN_LED_YELLOW, HIGH);
      break;

    case STATE_PASS_READY:
    case STATE_RUNNING:
      digitalWrite(PIN_LED_GREEN, HIGH);
      break;

    case STATE_FAIL_LOCKED:
      digitalWrite(PIN_LED_RED, HIGH);
      break;
  }
}

void drawCenteredText(const String& line1, const String& line2 = "", const String& line3 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  int y = 8;
  if (line1.length()) {
    display.setCursor(0, y);
    display.println(line1);
    y += 18;
  }
  if (line2.length()) {
    display.setCursor(0, y);
    display.println(line2);
    y += 18;
  }
  if (line3.length()) {
    display.setCursor(0, y);
    display.println(line3);
  }
  display.display();
}

void drawStatusScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Alcohol Interlock");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  switch (state) {
    case STATE_PREHEAT: {
      uint32_t elapsed = millis() - preheatStartMs;
      uint32_t remainingSec = (PREHEAT_MS > elapsed) ? ((PREHEAT_MS - elapsed + 999) / 1000) : 0;
      display.setCursor(0, 16);
      display.println("Khoi dong cam bien");
      display.setCursor(0, 28);
      display.print("Con lai: ");
      display.print(remainingSec);
      display.println("s");
      display.setCursor(0, 46);
      display.println("LED vang dang sang");
      break;
    }

    case STATE_STANDBY_LOCKED:
      display.setCursor(0, 16);
      display.println("San sang kiem tra");
      display.setCursor(0, 28);
      display.println("Nhan TEST de do con");
      display.setCursor(0, 46);
      display.println("Dong co dang khoa");
      break;

    case STATE_SAMPLING:
      display.setCursor(0, 16);
      display.println("Dang lay mau hoi tho");
      display.setCursor(0, 28);
      display.println("Vui long cho 1 giay");
      display.setCursor(0, 46);
      display.println("Dang phan tich...");
      break;

    case STATE_PASS_READY:
      display.setCursor(0, 16);
      display.println("KET QUA: AN TOAN");
      display.setCursor(0, 28);
      display.print("ADC: ");
      display.print(sampledAlcoholRaw);
      display.setCursor(0, 40);
      display.print("Muc demo: ");
      display.print(rawToPercent(sampledAlcoholRaw), 1);
      display.println("%");
      display.setCursor(0, 52);
      display.println("Nhan START de mo khoa");
      break;

    case STATE_FAIL_LOCKED:
      display.setCursor(0, 16);
      display.println("KET QUA: VI PHAM");
      display.setCursor(0, 28);
      display.print("ADC: ");
      display.print(sampledAlcoholRaw);
      display.setCursor(0, 40);
      display.print("Nguong: ");
      display.print(ALCOHOL_THRESHOLD);
      display.setCursor(0, 52);
      display.println("TEST lai de thu lai");
      break;

    case STATE_RUNNING:
      display.setCursor(0, 16);
      display.println("XE DANG CHAY");
      display.setCursor(0, 28);
      display.println("Servo mo khoa");
      display.setCursor(0, 40);
      display.println("TEST bi vo hieu hoa");
      display.setCursor(0, 52);
      display.println("Nhan START de dung");
      break;
  }

  display.display();
}

bool buttonPressed(ButtonTracker &btn) {
  bool reading = digitalRead(btn.pin);

  if (reading != btn.lastRead) {
    btn.lastChangeMs = millis();
    btn.lastRead = reading;
  }

  if ((millis() - btn.lastChangeMs) > BUTTON_DEBOUNCE_MS && reading != btn.lastStable) {
    btn.lastStable = reading;
    if (btn.lastStable == LOW) {
      return true;
    }
  }

  return false;
}

void shortBeep(uint16_t durationMs = 120) {
  setBuzzer(true);
  delay(durationMs);
  setBuzzer(false);
}

uint16_t sampleAlcoholAverage() {
  uint32_t sum = 0;
  uint32_t perSampleDelay = SAMPLE_TOTAL_MS / SAMPLE_COUNT;

  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    uint16_t raw = readAlcoholRaw();
    sum += raw;
    delay(perSampleDelay);
  }

  return (uint16_t)(sum / SAMPLE_COUNT);
}

void transitionTo(SystemState newState) {
  state = newState;
  applyIndicators();

  switch (state) {
    case STATE_PREHEAT:
      setBuzzer(false);
      lockVehicle();
      break;

    case STATE_STANDBY_LOCKED:
      setBuzzer(false);
      lockVehicle();
      break;

    case STATE_SAMPLING:
      setBuzzer(false);
      lockVehicle();
      break;

    case STATE_PASS_READY:
      setBuzzer(false);
      lockVehicle();
      shortBeep(180);
      break;

    case STATE_FAIL_LOCKED:
      lockVehicle();
      buzzerFailState = false;
      lastFailBuzzerToggleMs = 0;
      break;

    case STATE_RUNNING:
      setBuzzer(false);
      unlockVehicle();
      break;
  }

  drawStatusScreen();
}

void handlePreheat() {
  if (millis() - preheatStartMs >= PREHEAT_MS) {
    transitionTo(STATE_STANDBY_LOCKED);
  }
}

void handleSampling() {
  sampledAlcoholRaw = sampleAlcoholAverage();
  lastAlcoholRaw = sampledAlcoholRaw;

  Serial.println("====================");
  Serial.println("KET QUA DO CON");
  Serial.print("ADC trung binh: ");
  Serial.println(sampledAlcoholRaw);
  Serial.print("Muc demo (%): ");
  Serial.println(rawToPercent(sampledAlcoholRaw), 2);
  Serial.print("Nguong: ");
  Serial.println(ALCOHOL_THRESHOLD);

  if (sampledAlcoholRaw < ALCOHOL_THRESHOLD) {
    Serial.println("=> PASS - CHO PHEP KHOI DONG");
    transitionTo(STATE_PASS_READY);
  } else {
    Serial.println("=> FAIL - KHOA DONG CO");
    transitionTo(STATE_FAIL_LOCKED);
  }
}

void handleFailBuzzer() {
  uint32_t now = millis();
  uint32_t phase = now % FAIL_BUZZER_PERIOD;
  bool shouldOn = phase < FAIL_BUZZER_ON_MS;
  setBuzzer(shouldOn);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Alcohol Interlock...");

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_TEST, INPUT_PULLUP);
  pinMode(PIN_BTN_START, INPUT_PULLUP);

  if (ENABLE_RELAY_OUTPUT) {
    pinMode(PIN_RELAY, OUTPUT);
  }

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_MQ3, ADC_11db);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  lockServo.setPeriodHertz(50);
  lockServo.attach(PIN_SERVO, 500, 2400);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed!");
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.display();

  preheatStartMs = millis();
  transitionTo(STATE_PREHEAT);
}

void loop() {
  bool testPressed = buttonPressed(btnTest);
  bool startPressed = buttonPressed(btnStart);

  lastAlcoholRaw = readAlcoholRaw();

  switch (state) {
    case STATE_PREHEAT:
      handlePreheat();
      break;

    case STATE_STANDBY_LOCKED:
      if (testPressed) {
        transitionTo(STATE_SAMPLING);
        handleSampling();
      }
      break;

    case STATE_SAMPLING:

      break;

    case STATE_PASS_READY:
      if (startPressed) {
        Serial.println("START duoc phep -> Mo khoa / Van hanh");
        transitionTo(STATE_RUNNING);
      }
      if (testPressed) {
        transitionTo(STATE_SAMPLING);
        handleSampling();
      }
      break;

    case STATE_FAIL_LOCKED:
      handleFailBuzzer();
      if (testPressed) {
        transitionTo(STATE_SAMPLING);
        handleSampling();
      }
      break;

    case STATE_RUNNING:

      if (startPressed) {
        Serial.println("Da dung xe -> Ve trang thai khoa");
        transitionTo(STATE_STANDBY_LOCKED);
      }
      break;
  }

  if (millis() - lastUiRefreshMs >= UI_REFRESH_MS) {
    lastUiRefreshMs = millis();
    drawStatusScreen();
  }
}