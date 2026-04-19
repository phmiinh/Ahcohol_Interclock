#pragma once

#include <Arduino.h>

namespace app {
namespace config {

namespace pins {
constexpr int kMq3 = 34;
constexpr int kServo = 15;
constexpr int kBuzzer = 13;
constexpr int kLedRed = 25;
constexpr int kLedGreen = 26;
constexpr int kLedYellow = 27;
constexpr int kButtonTest = 14;
constexpr int kButtonStart = 16;
constexpr int kRelay = 33;
constexpr int kOledSda = 21;
constexpr int kOledScl = 22;
}  // namespace pins

namespace display {
constexpr int kWidth = 128;
constexpr int kHeight = 64;
constexpr int kReset = -1;
constexpr uint8_t kI2cAddress = 0x3C;
}  // namespace display

namespace servo {
constexpr int kLockAngle = 0;
constexpr int kUnlockAngle = 90;
constexpr int kPeriodHz = 50;
constexpr int kMinPulseUs = 500;
constexpr int kMaxPulseUs = 2400;
}  // namespace servo

namespace features {
constexpr bool kDemoMode = true;
constexpr bool kEnableRelayOutput = false;
constexpr bool kEnableSensorDebug = true;
constexpr bool kEnableDashboardProtocol = true;
}  // namespace features

namespace thresholds {
constexpr uint16_t kAlcoholAdc = 2000;
constexpr int kAdcMin = 0;
constexpr int kAdcMax = 4095;
}  // namespace thresholds

namespace timing {
constexpr uint32_t kPreheatMs = features::kDemoMode ? 10000UL : 60000UL;
constexpr uint8_t kSampleCount = 20;
constexpr uint32_t kSampleTotalMs = 1000UL;
constexpr uint32_t kSampleIntervalMs = kSampleTotalMs / kSampleCount;
constexpr uint32_t kPassBeepMs = 180UL;
constexpr uint32_t kButtonDebounceMs = 60UL;
constexpr uint32_t kUiRefreshMs = 200UL;
constexpr uint32_t kFailBuzzerPeriodMs = 500UL;
constexpr uint32_t kFailBuzzerOnMs = 220UL;
constexpr uint32_t kSensorDebugMs = 400UL;
constexpr uint32_t kDashboardTelemetryMs = 500UL;
}  // namespace timing

namespace buttons {
constexpr bool kUse3PinModules = true;
constexpr bool kActiveHigh = true;
constexpr uint8_t kPinMode = kUse3PinModules ? INPUT : INPUT_PULLUP;
constexpr bool kActiveLevel = kActiveHigh ? HIGH : LOW;
constexpr bool kIdleLevel = kActiveHigh ? LOW : HIGH;
constexpr const char* kModeName = kUse3PinModules ? "module_3pin" : "pushbutton";
}  // namespace buttons

constexpr long kSerialBaudRate = 115200;

}  // namespace config
}  // namespace app
