#pragma once

#include <Arduino.h>

#include "config.h"

namespace app {

enum class SystemState : uint8_t {
  Preheat,
  StandbyLocked,
  Sampling,
  PassReady,
  FailLocked,
  Running,
  ErrorLocked
};

enum class FaultCode : uint8_t {
  None,
  OledInitFailed,
  SensorTimeout
};

enum class SensorWarning : uint8_t {
  None,
  SaturatedLow,
  SaturatedHigh
};

struct SampleStatus {
  bool active = false;
  uint8_t collected = 0;
  uint8_t total = config::timing::kSampleCount;
  uint32_t startedAtMs = 0;
  uint32_t completedAtMs = 0;
  uint32_t durationMs = 0;
  uint16_t latestRaw = 0;
  float stdDev = 0.0f;
};

struct AppSnapshot {
  SystemState state = SystemState::Preheat;
  FaultCode fault = FaultCode::None;
  SensorWarning sensorWarning = SensorWarning::None;
  bool displayReady = true;
  bool vehicleLocked = true;
  bool buzzerOn = false;
  int servoAngle = config::servo::kLockAngle;
  uint16_t liveAdc = 0;
  uint16_t sampledAdc = 0;
  uint16_t threshold = config::thresholds::kAlcoholAdc;
  bool canStart = false;
  uint32_t preheatRemainingMs = 0;
  bool demoMode = config::features::kDemoMode;
  bool buttonActiveHigh = config::buttons::kActiveHigh;
  const char* buttonMode = config::buttons::kModeName;
  const char* buttonBias = config::buttons::kBiasName;
  uint32_t uptimeMs = 0;
  uint32_t sensorWarningDurationMs = 0;
  uint32_t testToResultMs = 0;
  uint32_t passReadyToUnlockMs = 0;
  uint32_t startToUnlockMs = 0;
  uint32_t consecutiveFailCount = 0;
  SampleStatus sampling;
};

inline const char* stateToString(SystemState state) {
  switch (state) {
    case SystemState::Preheat:
      return "PREHEAT";
    case SystemState::StandbyLocked:
      return "STANDBY_LOCKED";
    case SystemState::Sampling:
      return "SAMPLING";
    case SystemState::PassReady:
      return "PASS_READY";
    case SystemState::FailLocked:
      return "FAIL_LOCKED";
    case SystemState::Running:
      return "RUNNING";
    case SystemState::ErrorLocked:
      return "ERROR_LOCKED";
  }

  return "UNKNOWN";
}

inline const char* resultToString(SystemState state) {
  switch (state) {
    case SystemState::PassReady:
    case SystemState::Running:
      return "PASS";
    case SystemState::FailLocked:
    case SystemState::ErrorLocked:
      return "FAIL";
    default:
      return "PENDING";
  }
}

inline const char* stateSeverity(SystemState state) {
  switch (state) {
    case SystemState::PassReady:
    case SystemState::Running:
      return "success";
    case SystemState::FailLocked:
      return "warning";
    case SystemState::ErrorLocked:
      return "critical";
    default:
      return "info";
  }
}

inline const char* stateMessage(SystemState state) {
  switch (state) {
    case SystemState::Preheat:
      return "Sensor preheat in progress";
    case SystemState::StandbyLocked:
      return "Vehicle locked and waiting for TEST";
    case SystemState::Sampling:
      return "Sampling alcohol level";
    case SystemState::PassReady:
      return "Alcohol level safe. START enabled";
    case SystemState::FailLocked:
      return "Alcohol level above threshold. Vehicle locked";
    case SystemState::Running:
      return "Vehicle unlocked and running";
    case SystemState::ErrorLocked:
      return "System fault detected. Vehicle locked";
  }

  return "Unknown system state";
}

inline const char* faultToString(FaultCode fault) {
  switch (fault) {
    case FaultCode::None:
      return "NONE";
    case FaultCode::OledInitFailed:
      return "OLED_INIT_FAILED";
    case FaultCode::SensorTimeout:
      return "SENSOR_TIMEOUT";
  }

  return "UNKNOWN_FAULT";
}

inline const char* sensorWarningToString(SensorWarning warning) {
  switch (warning) {
    case SensorWarning::None:
      return "NONE";
    case SensorWarning::SaturatedLow:
      return "SATURATED_LOW";
    case SensorWarning::SaturatedHigh:
      return "SATURATED_HIGH";
  }

  return "UNKNOWN_WARNING";
}

inline float rawToPercent(uint16_t raw) {
  return (static_cast<float>(raw) / static_cast<float>(config::thresholds::kAdcMax)) * 100.0f;
}

}  // namespace app
