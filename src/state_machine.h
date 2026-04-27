#pragma once

#include "io_devices.h"
#include "telemetry.h"
#include "ui_oled.h"

namespace app {

class AlcoholInterlockController {
 public:
  void begin();
  void update();

 private:
  struct PassBeep {
    bool active = false;
    uint32_t startedAtMs = 0;
    uint16_t durationMs = 0;
  };

  struct SamplingSession {
    bool active = false;
    bool allNearLowRail = true;
    bool allNearHighRail = true;
    uint8_t collected = 0;
    uint32_t sum = 0;
    uint64_t sumSquares = 0;
    uint32_t startedAtMs = 0;
    uint32_t completedAtMs = 0;
    uint32_t nextSampleAtMs = 0;
    uint16_t lastRaw = 0;
  };

  void transitionTo(SystemState newState);
  void enterFault(FaultCode fault, const char* message);
  void refreshLiveAdc(uint32_t nowMs);
  void updateSensorHealth(uint32_t nowMs);
  void updatePreheat(uint32_t nowMs);
  void startSampling(uint32_t nowMs, bool whileRunning = false);
  void updateSampling(uint32_t nowMs);
  void finalizeSampling(uint32_t nowMs);
  void updateRunningRetestTimer(uint32_t nowMs);
  void updateRetestRequired(uint32_t nowMs, bool testPressed, bool startPressed);
  void updatePassBeep(uint32_t nowMs);
  void updateFailBuzzer(uint32_t nowMs);
  void updateRetestBuzzer(uint32_t nowMs);
  void updateUi(uint32_t nowMs, bool force = false);
  AppSnapshot snapshot(uint32_t nowMs) const;

  IoDevices io_;
  OledUi ui_;
  Telemetry telemetry_;
  SystemState state_ = SystemState::Preheat;
  FaultCode fault_ = FaultCode::None;
  SensorWarning sensorWarning_ = SensorWarning::None;
  SensorWarning sensorCandidateWarning_ = SensorWarning::None;
  uint32_t preheatStartedAtMs_ = 0;
  uint32_t sensorCandidateStartedAtMs_ = 0;
  uint32_t sensorWarningSinceMs_ = 0;
  uint32_t passReadyEnteredAtMs_ = 0;
  uint32_t lastUiRefreshMs_ = 0;
  uint32_t lastTestToResultMs_ = 0;
  uint32_t lastPassReadyToUnlockMs_ = 0;
  uint32_t lastStartToUnlockMs_ = 0;
  uint32_t consecutiveFailCount_ = 0;
  uint32_t runningSessionStartedAtMs_ = 0;
  uint32_t nextRetestAtMs_ = 0;
  uint32_t retestRequiredAtMs_ = 0;
  uint32_t retestCycleCount_ = 0;
  uint32_t lastRetestDueToTestMs_ = 0;
  uint32_t lastRetestToResultMs_ = 0;
  bool samplingWhileRunning_ = false;
  bool startReleasedAfterPassReady_ = false;
  uint16_t lastAlcoholRaw_ = 0;
  uint16_t sampledAlcoholRaw_ = 0;
  float sampledStdDev_ = 0.0f;
  PassBeep passBeep_;
  SamplingSession sampling_;
};

}  // namespace app
