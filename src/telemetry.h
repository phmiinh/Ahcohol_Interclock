#pragma once

#include "app_types.h"

namespace app {

class Telemetry {
 public:
  void begin();
  void logBoot();
  void logButtonConfig();
  void logStateChange(const AppSnapshot& snapshot);
  void logAction(const char* message);
  void logSamplingStarted(const AppSnapshot& snapshot);
  void logSamplingProgress(uint8_t sampleIndex, uint8_t totalSamples, uint16_t raw, uint32_t elapsedMs);
  void logSamplingResult(const AppSnapshot& snapshot, uint32_t durationMs, bool passed);
  void logFault(FaultCode fault, const char* message);
  void printLiveSensorDebug(const AppSnapshot& snapshot);
  void emitDashboardEvent(const char* eventName, const char* severity, const char* message, const AppSnapshot& snapshot);
  void emitDashboardTelemetry(const AppSnapshot& snapshot, bool force = false);

 private:
  uint32_t lastSensorDebugMs_ = 0;
  uint32_t lastDashboardTelemetryMs_ = 0;
};

}  // namespace app
