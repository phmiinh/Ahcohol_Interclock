#include "telemetry.h"

namespace app {
namespace {

void printPrefix(const char* level) {
  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");
  Serial.print(level);
  Serial.print(": ");
}

uint32_t sampleElapsedMs(const AppSnapshot& snapshot) {
  if (snapshot.sampling.startedAtMs == 0) {
    return 0;
  }

  if (snapshot.sampling.active && snapshot.uptimeMs >= snapshot.sampling.startedAtMs) {
    return snapshot.uptimeMs - snapshot.sampling.startedAtMs;
  }

  if (snapshot.sampling.completedAtMs >= snapshot.sampling.startedAtMs) {
    return snapshot.sampling.completedAtMs - snapshot.sampling.startedAtMs;
  }

  return 0;
}

}  // namespace

void Telemetry::begin() {
  Serial.begin(config::kSerialBaudRate);
  Serial.println();
}

void Telemetry::logBoot() {
  printPrefix("BOOT");
  Serial.println("Alcohol Interlock controller starting");
  printPrefix("BOOT");
  Serial.println("Mode: ESP32 Arduino + PlatformIO + Wokwi local");
  printPrefix("BOOT");
  Serial.println("Firmware source of truth: src/*");
  printPrefix("BOOT");
  Serial.print("Pins TEST=");
  Serial.print(config::pins::kButtonTest);
  Serial.print(" START=");
  Serial.print(config::pins::kButtonStart);
  Serial.print(" MQ3=");
  Serial.print(config::pins::kMq3);
  Serial.print(" SERVO=");
  Serial.println(config::pins::kServo);
}

void Telemetry::logButtonConfig() {
  printPrefix("BOOT");
  Serial.print("Buttons: ");
  Serial.print(config::buttons::kModeName);
  Serial.print(" | active ");
  Serial.print(config::buttons::kActiveHigh ? "HIGH" : "LOW");
  Serial.print(" | bias ");
  Serial.println(config::buttons::kBiasName);

  printPrefix("BOOT");
  Serial.println("TEST/START inputs use GPIO interrupt + software debounce");

  printPrefix("BOOT");
  Serial.println("Buzzer uses LEDC hardware timer at 2 kHz");

  printPrefix("BOOT");
  Serial.print("Running retest interval=");
  Serial.print(config::timing::kRetestIntervalMs);
  Serial.print(" ms | grace=");
  Serial.print(config::timing::kRetestGraceMs);
  Serial.println(" ms");

  printPrefix("BOOT");
  Serial.print("Serial mode: ");
  Serial.println(config::features::kEnableDashboardProtocol ? "dashboard AI_JSON stream" : "quiet Wokwi monitor");
}

void Telemetry::logStateChange(const AppSnapshot& snapshot) {
  printPrefix("STATE");
  Serial.print("Transition -> ");
  Serial.print(stateToString(snapshot.state));
  Serial.print(" | locked=");
  Serial.print(snapshot.vehicleLocked ? "true" : "false");
  Serial.print(" | buzzer=");
  Serial.print(snapshot.buzzerOn ? "true" : "false");
  Serial.print(" | liveAdc=");
  Serial.println(snapshot.liveAdc);
}

void Telemetry::logAction(const char* message) {
  printPrefix("ACTION");
  Serial.println(message);
}

void Telemetry::logSamplingStarted(const AppSnapshot& snapshot) {
  printPrefix("SAMPLE");
  Serial.print("Window started | count=");
  Serial.print(snapshot.sampling.total);
  Serial.print(" | duration=");
  Serial.print(config::timing::kSampleTotalMs);
  Serial.print(" ms | interval=");
  Serial.print(config::timing::kSampleIntervalMs);
  Serial.print(" ms | threshold=");
  Serial.println(snapshot.threshold);
}

void Telemetry::logSamplingProgress(uint8_t sampleIndex, uint8_t totalSamples, uint16_t raw, uint32_t elapsedMs) {
  if (!config::features::kEnableSampleProgressLog) {
    return;
  }

  printPrefix("SAMPLE");
  Serial.print(sampleIndex);
  Serial.print("/");
  Serial.print(totalSamples);
  Serial.print(" | adc=");
  Serial.print(raw);
  Serial.print(" | elapsed=");
  Serial.print(elapsedMs);
  Serial.println(" ms");
}

void Telemetry::logSamplingResult(const AppSnapshot& snapshot, uint32_t durationMs, bool passed) {
  printPrefix("RESULT");
  Serial.print("Sampling finished in ");
  Serial.print(durationMs);
  Serial.println(" ms");

  printPrefix("RESULT");
  Serial.print("Average ADC=");
  Serial.print(snapshot.sampledAdc);
  Serial.print(" | stddev=");
  Serial.print(snapshot.sampling.stdDev, 1);
  Serial.print(" | threshold=");
  Serial.print(snapshot.threshold);
  Serial.print(" | samples=");
  Serial.print(snapshot.sampling.collected);
  Serial.print("/");
  Serial.print(snapshot.sampling.total);
  Serial.print(" | result=");
  Serial.println(passed ? "PASS" : "FAIL");

  printPrefix("METRIC");
  Serial.print("test_to_result_ms=");
  Serial.print(snapshot.testToResultMs);
  Serial.print(" | consecutive_fail_count=");
  Serial.println(snapshot.consecutiveFailCount);

  if (snapshot.retestRequired || snapshot.retestToResultMs > 0) {
    printPrefix("METRIC");
    Serial.print("retest_due_to_test_ms=");
    Serial.print(snapshot.retestDueToTestMs);
    Serial.print(" | retest_to_result_ms=");
    Serial.print(snapshot.retestToResultMs);
    Serial.print(" | next_retest_in_ms=");
    Serial.println(config::timing::kRetestIntervalMs);
  }
}

void Telemetry::logStartUnlockMetrics(const AppSnapshot& snapshot) {
  printPrefix("METRIC");
  Serial.print("pass_ready_to_unlock_ms=");
  Serial.print(snapshot.passReadyToUnlockMs);
  Serial.print(" | start_to_unlock_ms=");
  Serial.println(snapshot.startToUnlockMs);
}

void Telemetry::logSensorWarning(SensorWarning warning, uint16_t raw, uint32_t durationMs) {
  printPrefix("WARN");
  Serial.print(sensorWarningToString(warning));
  Serial.print(" | raw=");
  Serial.print(raw);
  Serial.print(" | duration=");
  Serial.print(durationMs);
  Serial.println(" ms");
}

void Telemetry::logSensorRecovered(uint16_t raw) {
  printPrefix("INFO");
  Serial.print("Sensor warning cleared | raw=");
  Serial.println(raw);
}

void Telemetry::logFault(FaultCode fault, const char* message) {
  printPrefix("FAULT");
  Serial.print(faultToString(fault));
  Serial.print(" | ");
  Serial.println(message);
}

void Telemetry::printLiveSensorDebug(const AppSnapshot& snapshot) {
  if (!config::features::kEnableSensorDebug) {
    return;
  }

  if (snapshot.uptimeMs - lastSensorDebugMs_ < config::timing::kSensorDebugMs) {
    return;
  }

  lastSensorDebugMs_ = snapshot.uptimeMs;

  printPrefix("ADC");
  Serial.print("live=");
  Serial.print(snapshot.liveAdc);
  Serial.print(" | threshold=");
  Serial.print(snapshot.threshold);
  Serial.print(" | status=");
  Serial.println(snapshot.liveAdc < snapshot.threshold ? "SAFE" : "HIGH");
}

void Telemetry::emitDashboardEvent(
    const char* eventName,
    const char* severity,
    const char* message,
    const AppSnapshot& snapshot) {
  if (!config::features::kEnableDashboardProtocol) {
    return;
  }

  Serial.print("AI_JSON {\"type\":\"event\",\"event\":\"");
  Serial.print(eventName);
  Serial.print("\",\"severity\":\"");
  Serial.print(severity);
  Serial.print("\",\"message\":\"");
  Serial.print(message);
  Serial.print("\",\"state\":\"");
  Serial.print(stateToString(snapshot.state));
  Serial.print("\",\"fault\":\"");
  Serial.print(faultToString(snapshot.fault));
  Serial.print("\",\"sensorWarning\":\"");
  Serial.print(sensorWarningToString(snapshot.sensorWarning));
  Serial.print("\",\"displayReady\":");
  Serial.print(snapshot.displayReady ? "true" : "false");
  Serial.print(",\"liveAdc\":");
  Serial.print(snapshot.liveAdc);
  Serial.print(",\"sampledAdc\":");
  Serial.print(snapshot.sampledAdc);
  Serial.print(",\"threshold\":");
  Serial.print(snapshot.threshold);
  Serial.print(",\"vehicleLocked\":");
  Serial.print(snapshot.vehicleLocked ? "true" : "false");
  Serial.print(",\"buzzerOn\":");
  Serial.print(snapshot.buzzerOn ? "true" : "false");
  Serial.print(",\"servoAngle\":");
  Serial.print(snapshot.servoAngle);
  Serial.print(",\"testToResultMs\":");
  Serial.print(snapshot.testToResultMs);
  Serial.print(",\"passReadyToUnlockMs\":");
  Serial.print(snapshot.passReadyToUnlockMs);
  Serial.print(",\"startToUnlockMs\":");
  Serial.print(snapshot.startToUnlockMs);
  Serial.print(",\"consecutiveFailCount\":");
  Serial.print(snapshot.consecutiveFailCount);
  Serial.print(",\"retestRequired\":");
  Serial.print(snapshot.retestRequired ? "true" : "false");
  Serial.print(",\"retestIntervalMs\":");
  Serial.print(snapshot.retestIntervalMs);
  Serial.print(",\"retestRemainingMs\":");
  Serial.print(snapshot.retestRemainingMs);
  Serial.print(",\"retestOverdueMs\":");
  Serial.print(snapshot.retestOverdueMs);
  Serial.print(",\"retestGraceRemainingMs\":");
  Serial.print(snapshot.retestGraceRemainingMs);
  Serial.print(",\"retestDueToTestMs\":");
  Serial.print(snapshot.retestDueToTestMs);
  Serial.print(",\"retestToResultMs\":");
  Serial.print(snapshot.retestToResultMs);
  Serial.print(",\"runningSessionMs\":");
  Serial.print(snapshot.runningSessionMs);
  Serial.print(",\"retestCycleCount\":");
  Serial.print(snapshot.retestCycleCount);
  Serial.print(",\"uptimeMs\":");
  Serial.print(snapshot.uptimeMs);
  Serial.println("}");
}

void Telemetry::emitDashboardTelemetry(const AppSnapshot& snapshot, bool force) {
  if (!config::features::kEnableDashboardProtocol) {
    return;
  }

  if (!force && snapshot.uptimeMs - lastDashboardTelemetryMs_ < config::timing::kDashboardTelemetryMs) {
    return;
  }

  lastDashboardTelemetryMs_ = snapshot.uptimeMs;

  Serial.print("AI_JSON {\"type\":\"telemetry\",\"state\":\"");
  Serial.print(stateToString(snapshot.state));
  Serial.print("\",\"result\":\"");
  Serial.print(resultToString(snapshot.state));
  Serial.print("\",\"fault\":\"");
  Serial.print(faultToString(snapshot.fault));
  Serial.print("\",\"sensorWarning\":\"");
  Serial.print(sensorWarningToString(snapshot.sensorWarning));
  Serial.print("\",\"displayReady\":");
  Serial.print(snapshot.displayReady ? "true" : "false");
  Serial.print(",\"liveAdc\":");
  Serial.print(snapshot.liveAdc);
  Serial.print(",\"sampledAdc\":");
  Serial.print(snapshot.sampledAdc);
  Serial.print(",\"threshold\":");
  Serial.print(snapshot.threshold);
  Serial.print(",\"percent\":");
  Serial.print(rawToPercent(snapshot.liveAdc), 1);
  Serial.print(",\"samplePercent\":");
  Serial.print(rawToPercent(snapshot.sampledAdc), 1);
  Serial.print(",\"sampleStdDev\":");
  Serial.print(snapshot.sampling.stdDev, 1);
  Serial.print(",\"preheatRemainingMs\":");
  Serial.print(snapshot.preheatRemainingMs);
  Serial.print(",\"demoMode\":");
  Serial.print(snapshot.demoMode ? "true" : "false");
  Serial.print(",\"canStart\":");
  Serial.print(snapshot.canStart ? "true" : "false");
  Serial.print(",\"vehicleLocked\":");
  Serial.print(snapshot.vehicleLocked ? "true" : "false");
  Serial.print(",\"buzzerOn\":");
  Serial.print(snapshot.buzzerOn ? "true" : "false");
  Serial.print(",\"servoAngle\":");
  Serial.print(snapshot.servoAngle);
  Serial.print(",\"buttonMode\":\"");
  Serial.print(snapshot.buttonMode);
  Serial.print("\",\"buttonActiveHigh\":");
  Serial.print(snapshot.buttonActiveHigh ? "true" : "false");
  Serial.print(",\"buttonBias\":\"");
  Serial.print(snapshot.buttonBias);
  Serial.print("\"");
  Serial.print(",\"sensorWarningDurationMs\":");
  Serial.print(snapshot.sensorWarningDurationMs);
  Serial.print(",\"sampleCount\":");
  Serial.print(snapshot.sampling.collected);
  Serial.print(",\"sampleTotal\":");
  Serial.print(snapshot.sampling.total);
  Serial.print(",\"sampleElapsedMs\":");
  Serial.print(sampleElapsedMs(snapshot));
  Serial.print(",\"testToResultMs\":");
  Serial.print(snapshot.testToResultMs);
  Serial.print(",\"passReadyToUnlockMs\":");
  Serial.print(snapshot.passReadyToUnlockMs);
  Serial.print(",\"startToUnlockMs\":");
  Serial.print(snapshot.startToUnlockMs);
  Serial.print(",\"consecutiveFailCount\":");
  Serial.print(snapshot.consecutiveFailCount);
  Serial.print(",\"retestRequired\":");
  Serial.print(snapshot.retestRequired ? "true" : "false");
  Serial.print(",\"retestIntervalMs\":");
  Serial.print(snapshot.retestIntervalMs);
  Serial.print(",\"retestRemainingMs\":");
  Serial.print(snapshot.retestRemainingMs);
  Serial.print(",\"retestOverdueMs\":");
  Serial.print(snapshot.retestOverdueMs);
  Serial.print(",\"retestGraceRemainingMs\":");
  Serial.print(snapshot.retestGraceRemainingMs);
  Serial.print(",\"retestDueToTestMs\":");
  Serial.print(snapshot.retestDueToTestMs);
  Serial.print(",\"retestToResultMs\":");
  Serial.print(snapshot.retestToResultMs);
  Serial.print(",\"runningSessionMs\":");
  Serial.print(snapshot.runningSessionMs);
  Serial.print(",\"retestCycleCount\":");
  Serial.print(snapshot.retestCycleCount);
  Serial.print(",\"uptimeMs\":");
  Serial.print(snapshot.uptimeMs);
  Serial.println("}");
}

}  // namespace app
