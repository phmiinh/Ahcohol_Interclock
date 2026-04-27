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
// Keep Wokwi Serial Monitor readable by default. Enable these when collecting
// detailed telemetry or using the realtime dashboard with a physical ESP32.
constexpr bool kEnableSensorDebug = false;
constexpr bool kEnableDashboardProtocol = false;
constexpr bool kEnableSampleProgressLog = false;
// In Wokwi, users often turn the potentiometer fully to 0 or 4095 to demo
// PASS/FAIL. Treat that as a warning in demo mode, but keep strict fail-safe
// behavior available for production hardware.
constexpr bool kFaultOnAllRailSamples = !kDemoMode;
}  // namespace features

namespace thresholds {
// Demo threshold for Wokwi / presentation flow. Re-calibrate when using a real MQ3.
constexpr uint16_t kAlcoholAdc = 2000;
constexpr int kAdcMin = 0;
constexpr int kAdcMax = 4095;
constexpr uint16_t kRailLowAdc = 1;
constexpr uint16_t kRailHighAdc = 4094;
}  // namespace thresholds

namespace timing {
// Demo preheat for faster presentation. Real hardware may need longer warm-up.
constexpr uint32_t kPreheatMs = features::kDemoMode ? 10000UL : 60000UL;
// Sampling window for TEST/retest. Tune again if you move to a calibrated MQ3 setup.
constexpr uint8_t kSampleCount = 20;
constexpr uint32_t kSampleTotalMs = 2000UL;
constexpr uint32_t kSampleIntervalMs = kSampleCount > 1 ? (kSampleTotalMs / (kSampleCount - 1)) : kSampleTotalMs;
// Rolling retest target is 30 minutes in production. Demo mode shortens it so the flow is testable in Wokwi.
constexpr uint32_t kRetestProductionMs = 30UL * 60UL * 1000UL;
constexpr uint32_t kRetestDemoMs = 30UL * 1000UL;
constexpr uint32_t kRetestIntervalMs = features::kDemoMode ? kRetestDemoMs : kRetestProductionMs;
// Grace window before safe-lock if the operator ignores a due retest.
constexpr uint32_t kRetestGraceMs = features::kDemoMode ? 15UL * 1000UL : 5UL * 60UL * 1000UL;
constexpr uint32_t kPassBeepMs = 180UL;
constexpr uint32_t kButtonDebounceMs = 60UL;
constexpr uint32_t kRunningStartRelockGuardMs = 800UL;
constexpr uint32_t kUiRefreshMs = 200UL;
constexpr uint32_t kFailBuzzerPeriodMs = 500UL;
constexpr uint32_t kFailBuzzerOnMs = 220UL;
constexpr uint32_t kRetestBuzzerPeriodMs = 1200UL;
constexpr uint32_t kRetestBuzzerOnMs = 120UL;
constexpr uint32_t kSensorDebugMs = 400UL;
constexpr uint32_t kSensorRailWarnMs = 8000UL;
constexpr uint32_t kDashboardTelemetryMs = 500UL;
}  // namespace timing

namespace buttons {
// Firmware logic targets an active-HIGH button model.
// Wokwi uses plain pushbuttons plus 10k pulldown resistors to emulate the OUT pin of an active-HIGH button module.
// Real hardware may use a 3-pin module or a discrete pushbutton with an equivalent bias network.
constexpr bool kUse3PinModules = true;
constexpr bool kActiveHigh = true;
constexpr bool kUseInternalBias = true;
// Keep the line at a stable idle level even if the external button module output floats.
constexpr uint8_t kPinMode = kUseInternalBias ? (kActiveHigh ? INPUT_PULLDOWN : INPUT_PULLUP) : INPUT;
constexpr bool kActiveLevel = kActiveHigh ? HIGH : LOW;
constexpr bool kIdleLevel = kActiveHigh ? LOW : HIGH;
constexpr const char* kModeName = kUse3PinModules ? "module_3pin" : "pushbutton";
constexpr const char* kBiasName = kUseInternalBias ? (kActiveHigh ? "internal_pulldown" : "internal_pullup")
                                                   : "external_bias";
}  // namespace buttons

constexpr long kSerialBaudRate = 115200;

}  // namespace config
}  // namespace app
