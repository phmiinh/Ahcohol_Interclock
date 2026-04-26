#include "state_machine.h"

#include <math.h>

namespace app {
namespace {

const char* sensorWarningMessage(SensorWarning warning) {
  switch (warning) {
    case SensorWarning::SaturatedLow:
      return "ADC stays near 0 for too long. Check sensor path or input bias.";
    case SensorWarning::SaturatedHigh:
      return "ADC stays near 4095 for too long. Check MQ3 simulation or sensor saturation.";
    case SensorWarning::None:
      return "Sensor operating in normal ADC range.";
  }

  return "Unknown sensor status.";
}

}  // namespace

void AlcoholInterlockController::begin() {
  telemetry_.begin();
  telemetry_.logBoot();
  telemetry_.logButtonConfig();

  io_.begin();
  preheatStartedAtMs_ = millis();
  lastUiRefreshMs_ = preheatStartedAtMs_;

  if (!io_.isDisplayReady()) {
    enterFault(FaultCode::OledInitFailed, "OLED init failed. System locked");
    return;
  }

  transitionTo(SystemState::Preheat);
  telemetry_.emitDashboardEvent("boot", "info", "Device booted", snapshot(millis()));
  telemetry_.emitDashboardTelemetry(snapshot(millis()), true);
}

void AlcoholInterlockController::update() {
  const uint32_t nowMs = millis();

  if (fault_ == FaultCode::None) {
    refreshLiveAdc(nowMs);
  } else {
    updateUi(nowMs, true);
  }

  const AppSnapshot liveSnapshot = snapshot(nowMs);
  telemetry_.printLiveSensorDebug(liveSnapshot);
  telemetry_.emitDashboardTelemetry(liveSnapshot);

  const bool testPressed = io_.buttonPressed(ButtonId::Test, nowMs);
  const bool startPressed = io_.buttonPressed(ButtonId::Start, nowMs);

  switch (state_) {
    case SystemState::Preheat:
      updatePreheat(nowMs);
      break;

    case SystemState::StandbyLocked:
      if (testPressed) {
        telemetry_.logAction("TEST pressed. Starting sampling window");
        startSampling(nowMs);
      }
      break;

    case SystemState::Sampling:
    case SystemState::RetestSampling:
      updateSampling(nowMs);
      break;

    case SystemState::PassReady:
      if (startPressed) {
        const uint32_t startPressedAtMs = nowMs;
        lastPassReadyToUnlockMs_ = passReadyEnteredAtMs_ > 0 ? (startPressedAtMs - passReadyEnteredAtMs_) : 0;
        telemetry_.logAction("START accepted. Vehicle unlocked");
        transitionTo(SystemState::Running);
        lastStartToUnlockMs_ = millis() - startPressedAtMs;
        const AppSnapshot runningSnapshot = snapshot(millis());
        telemetry_.logStartUnlockMetrics(runningSnapshot);
        telemetry_.emitDashboardEvent("start_unlocked", "success", "START accepted. Servo unlocked", runningSnapshot);
      } else if (testPressed) {
        telemetry_.logAction("TEST pressed. Re-sampling");
        startSampling(nowMs);
      }
      break;

    case SystemState::FailLocked:
      if (testPressed) {
        telemetry_.logAction("TEST pressed from FAIL state. Re-sampling");
        startSampling(nowMs);
      }
      break;

    case SystemState::Running:
      updateRunningRetestTimer(nowMs);
      if (startPressed && runningSessionStartedAtMs_ > 0 &&
          nowMs - runningSessionStartedAtMs_ >= config::timing::kRunningStartRelockGuardMs) {
        telemetry_.logAction("START pressed while RUNNING. Returning to locked standby");
        transitionTo(SystemState::StandbyLocked);
      } else if (testPressed) {
        telemetry_.logAction("Manual running retest requested");
        startSampling(nowMs, true);
      }
      break;

    case SystemState::RetestRequired:
      updateRetestRequired(nowMs, testPressed, startPressed);
      break;

    case SystemState::ErrorLocked:
      break;
  }

  updatePassBeep(nowMs);
  if (state_ == SystemState::FailLocked) {
    updateFailBuzzer(nowMs);
  } else if (state_ == SystemState::RetestRequired) {
    updateRetestBuzzer(nowMs);
  } else if (!passBeep_.active) {
    io_.setBuzzer(false);
  }

  updateUi(nowMs);
}

void AlcoholInterlockController::transitionTo(SystemState newState) {
  const uint32_t transitionStartedAtMs = millis();
  const SystemState previousState = state_;
  state_ = newState;
  passBeep_ = {};

  if (state_ != SystemState::Sampling && state_ != SystemState::RetestSampling) {
    sampling_.active = false;
  }

  if (state_ == SystemState::PassReady) {
    passReadyEnteredAtMs_ = transitionStartedAtMs;
  } else if (state_ != SystemState::Running) {
    passReadyEnteredAtMs_ = 0;
  }

  if (state_ == SystemState::Running) {
    if (previousState != SystemState::RetestRequired && previousState != SystemState::RetestSampling) {
      runningSessionStartedAtMs_ = transitionStartedAtMs;
      retestCycleCount_ = 0;
    }
    nextRetestAtMs_ = transitionStartedAtMs + config::timing::kRetestIntervalMs;
    retestRequiredAtMs_ = 0;
    samplingWhileRunning_ = false;
  } else if (state_ == SystemState::RetestRequired) {
    retestRequiredAtMs_ = transitionStartedAtMs;
    ++retestCycleCount_;
  } else if (state_ != SystemState::Sampling && state_ != SystemState::RetestSampling) {
    runningSessionStartedAtMs_ = 0;
    nextRetestAtMs_ = 0;
    retestRequiredAtMs_ = 0;
    samplingWhileRunning_ = false;
  }

  switch (state_) {
    case SystemState::Preheat:
    case SystemState::StandbyLocked:
    case SystemState::PassReady:
      io_.setBuzzer(false);
      io_.lockVehicle();
      break;

    case SystemState::Sampling:
      io_.setBuzzer(false);
      io_.lockVehicle();
      break;

    case SystemState::RetestSampling:
      io_.setBuzzer(false);
      io_.unlockVehicle();
      break;

    case SystemState::FailLocked:
      io_.setBuzzer(false);
      io_.lockVehicle();
      break;

    case SystemState::Running:
      io_.setBuzzer(false);
      io_.unlockVehicle();
      break;

    case SystemState::RetestRequired:
      io_.setBuzzer(false);
      io_.unlockVehicle();
      break;

    case SystemState::ErrorLocked:
      io_.setBuzzer(false);
      io_.lockVehicle();
      break;
  }

  io_.setIndicators(state_);

  if (state_ == SystemState::PassReady) {
    passBeep_.active = true;
    passBeep_.startedAtMs = transitionStartedAtMs;
    passBeep_.durationMs = config::timing::kPassBeepMs;
    io_.setBuzzer(true);
  }

  const AppSnapshot current = snapshot(transitionStartedAtMs);
  telemetry_.logStateChange(current);
  telemetry_.emitDashboardEvent("state_changed", stateSeverity(state_), stateMessage(state_), current);
  telemetry_.emitDashboardTelemetry(current, true);
  updateUi(current.uptimeMs, true);
}

void AlcoholInterlockController::enterFault(FaultCode fault, const char* message) {
  fault_ = fault;
  sampling_.active = false;
  passBeep_ = {};
  transitionTo(SystemState::ErrorLocked);
  telemetry_.logFault(fault, message);
  telemetry_.emitDashboardEvent("fault", "critical", message, snapshot(millis()));
  telemetry_.emitDashboardTelemetry(snapshot(millis()), true);
}

void AlcoholInterlockController::refreshLiveAdc(uint32_t nowMs) {
  lastAlcoholRaw_ = io_.readAlcoholRaw();
  if (state_ != SystemState::Sampling && state_ != SystemState::RetestSampling) {
    updateSensorHealth(nowMs);
  }
}

void AlcoholInterlockController::updateSensorHealth(uint32_t nowMs) {
  SensorWarning candidate = SensorWarning::None;
  if (lastAlcoholRaw_ <= config::thresholds::kRailLowAdc) {
    candidate = SensorWarning::SaturatedLow;
  } else if (lastAlcoholRaw_ >= config::thresholds::kRailHighAdc) {
    candidate = SensorWarning::SaturatedHigh;
  }

  if (candidate != sensorCandidateWarning_) {
    sensorCandidateWarning_ = candidate;
    sensorCandidateStartedAtMs_ = nowMs;
  }

  if (candidate == SensorWarning::None) {
    if (sensorWarning_ != SensorWarning::None) {
      sensorWarning_ = SensorWarning::None;
      sensorWarningSinceMs_ = 0;
      telemetry_.logSensorRecovered(lastAlcoholRaw_);
      telemetry_.emitDashboardEvent(
          "sensor_warning_cleared",
          "info",
          "ADC returned to normal operating range.",
          snapshot(nowMs));
    }
    return;
  }

  if (nowMs - sensorCandidateStartedAtMs_ < config::timing::kSensorRailWarnMs) {
    return;
  }

  if (sensorWarning_ == candidate) {
    return;
  }

  sensorWarning_ = candidate;
  sensorWarningSinceMs_ = sensorCandidateStartedAtMs_;
  telemetry_.logSensorWarning(sensorWarning_, lastAlcoholRaw_, nowMs - sensorCandidateStartedAtMs_);
  telemetry_.emitDashboardEvent("sensor_warning", "warning", sensorWarningMessage(sensorWarning_), snapshot(nowMs));
}

void AlcoholInterlockController::updatePreheat(uint32_t nowMs) {
  if (nowMs - preheatStartedAtMs_ >= config::timing::kPreheatMs) {
    transitionTo(SystemState::StandbyLocked);
  }
}

void AlcoholInterlockController::startSampling(uint32_t nowMs, bool whileRunning) {
  sampling_ = {};
  sampling_.active = true;
  sampling_.startedAtMs = nowMs;
  sampling_.nextSampleAtMs = nowMs;
  samplingWhileRunning_ = whileRunning;
  sampledAlcoholRaw_ = 0;
  sampledStdDev_ = 0.0f;
  if (whileRunning) {
    lastRetestToResultMs_ = 0;
    lastRetestDueToTestMs_ = retestRequiredAtMs_ > 0 ? (nowMs - retestRequiredAtMs_) : 0;
  } else {
    lastTestToResultMs_ = 0;
  }
  transitionTo(whileRunning ? SystemState::RetestSampling : SystemState::Sampling);
  telemetry_.logSamplingStarted(snapshot(nowMs));
}

void AlcoholInterlockController::updateSampling(uint32_t nowMs) {
  if (!sampling_.active) {
    return;
  }

  // Intentionally take at most one sample per update cycle.
  // If loop() is delayed, skip missed slots instead of catching up in a burst.
  if (sampling_.collected < config::timing::kSampleCount && nowMs >= sampling_.nextSampleAtMs) {
    const uint16_t raw = io_.readAlcoholRaw();
    sampling_.sum += raw;
    sampling_.sumSquares += static_cast<uint64_t>(raw) * static_cast<uint64_t>(raw);
    sampling_.allNearLowRail = sampling_.allNearLowRail && raw <= config::thresholds::kRailLowAdc;
    sampling_.allNearHighRail = sampling_.allNearHighRail && raw >= config::thresholds::kRailHighAdc;
    sampling_.lastRaw = raw;
    lastAlcoholRaw_ = raw;
    ++sampling_.collected;
    sampling_.nextSampleAtMs = nowMs + config::timing::kSampleIntervalMs;

    telemetry_.logSamplingProgress(sampling_.collected, config::timing::kSampleCount, raw, nowMs - sampling_.startedAtMs);
  }

  if (sampling_.collected >= config::timing::kSampleCount) {
    finalizeSampling(nowMs);
  }
}

void AlcoholInterlockController::finalizeSampling(uint32_t nowMs) {
  sampling_.active = false;
  sampling_.completedAtMs = nowMs;
  sampledAlcoholRaw_ = static_cast<uint16_t>(sampling_.sum / config::timing::kSampleCount);
  lastAlcoholRaw_ = sampledAlcoholRaw_;
  const uint32_t resultDurationMs = nowMs - sampling_.startedAtMs;
  lastTestToResultMs_ = resultDurationMs;
  if (samplingWhileRunning_) {
    lastRetestToResultMs_ = resultDurationMs;
  }

  const float sampleMean = static_cast<float>(sampling_.sum) / static_cast<float>(config::timing::kSampleCount);
  const float sampleMeanSquares = static_cast<float>(sampling_.sumSquares) / static_cast<float>(config::timing::kSampleCount);
  const float sampleVariance = fmaxf(0.0f, sampleMeanSquares - (sampleMean * sampleMean));
  sampledStdDev_ = sqrtf(sampleVariance);

  const bool allSamplesNearRail = sampling_.allNearLowRail || sampling_.allNearHighRail;
  if (allSamplesNearRail && config::features::kFaultOnAllRailSamples) {
    enterFault(FaultCode::SensorTimeout, "All samples stuck near ADC rail. Check sensor wiring.");
    return;
  }

  if (allSamplesNearRail) {
    telemetry_.logSensorWarning(
        sampling_.allNearLowRail ? SensorWarning::SaturatedLow : SensorWarning::SaturatedHigh,
        sampledAlcoholRaw_,
        resultDurationMs);
  }

  const bool passed = sampledAlcoholRaw_ < config::thresholds::kAlcoholAdc;
  const bool wasRetest = samplingWhileRunning_;
  consecutiveFailCount_ = passed ? 0 : (consecutiveFailCount_ + 1);
  telemetry_.logSamplingResult(snapshot(nowMs), resultDurationMs, passed);

  transitionTo(passed ? (wasRetest ? SystemState::Running : SystemState::PassReady) : SystemState::FailLocked);
  telemetry_.emitDashboardEvent(
      "sample_result",
      passed ? "success" : "warning",
      passed ? (wasRetest ? "Periodic retest passed. Vehicle continues running" : "Alcohol level safe")
             : (wasRetest ? "Periodic retest failed. Vehicle locked" : "Alcohol level above threshold"),
      snapshot(millis()));
}

void AlcoholInterlockController::updateRunningRetestTimer(uint32_t nowMs) {
  if (nextRetestAtMs_ == 0 || nowMs < nextRetestAtMs_) {
    return;
  }

  telemetry_.logAction("Periodic retest required. Press TEST to continue running");
  transitionTo(SystemState::RetestRequired);
  telemetry_.emitDashboardEvent(
      "retest_required",
      "warning",
      config::features::kDemoMode ? "Demo retest is due. Press TEST to continue running."
                                  : "30-minute periodic retest is due. Press TEST to verify alcohol level again.",
      snapshot(millis()));
}

void AlcoholInterlockController::updateRetestRequired(uint32_t nowMs, bool testPressed, bool startPressed) {
  if (testPressed) {
    telemetry_.logAction("Periodic retest accepted. Sampling while vehicle remains running");
    startSampling(nowMs, true);
    return;
  }

  if (startPressed) {
    telemetry_.logAction("START pressed during retest request. Returning to locked standby");
    transitionTo(SystemState::StandbyLocked);
    return;
  }

  if (retestRequiredAtMs_ > 0 && nowMs - retestRequiredAtMs_ >= config::timing::kRetestGraceMs) {
    enterFault(FaultCode::RetestTimeout, "Periodic retest timeout. Vehicle locked in safe state.");
  }
}

void AlcoholInterlockController::updatePassBeep(uint32_t nowMs) {
  if (!passBeep_.active) {
    return;
  }

  if (nowMs - passBeep_.startedAtMs >= passBeep_.durationMs) {
    passBeep_.active = false;
    if (state_ != SystemState::FailLocked) {
      io_.setBuzzer(false);
    }
  }
}

void AlcoholInterlockController::updateFailBuzzer(uint32_t nowMs) {
  const uint32_t phase = nowMs % config::timing::kFailBuzzerPeriodMs;
  io_.setBuzzer(phase < config::timing::kFailBuzzerOnMs);
}

void AlcoholInterlockController::updateRetestBuzzer(uint32_t nowMs) {
  const uint32_t phase = nowMs % config::timing::kRetestBuzzerPeriodMs;
  io_.setBuzzer(phase < config::timing::kRetestBuzzerOnMs);
}

void AlcoholInterlockController::updateUi(uint32_t nowMs, bool force) {
  if (!force && nowMs - lastUiRefreshMs_ < config::timing::kUiRefreshMs) {
    return;
  }

  lastUiRefreshMs_ = nowMs;
  ui_.render(io_, snapshot(nowMs));
}

AppSnapshot AlcoholInterlockController::snapshot(uint32_t nowMs) const {
  AppSnapshot snapshot;
  snapshot.state = state_;
  snapshot.fault = fault_;
  snapshot.sensorWarning = sensorWarning_;
  snapshot.displayReady = io_.isDisplayReady();
  snapshot.vehicleLocked = io_.vehicleLocked();
  snapshot.buzzerOn = io_.buzzerOn();
  snapshot.servoAngle = io_.servoAngle();
  snapshot.liveAdc = lastAlcoholRaw_;
  snapshot.sampledAdc = sampledAlcoholRaw_;
  snapshot.threshold = config::thresholds::kAlcoholAdc;
  snapshot.canStart = state_ == SystemState::PassReady;
  snapshot.demoMode = config::features::kDemoMode;
  snapshot.buttonActiveHigh = config::buttons::kActiveHigh;
  snapshot.buttonMode = config::buttons::kModeName;
  snapshot.buttonBias = config::buttons::kBiasName;
  snapshot.uptimeMs = nowMs;
  snapshot.sensorWarningDurationMs = sensorWarningSinceMs_ > 0 ? (nowMs - sensorWarningSinceMs_) : 0;
  snapshot.testToResultMs = lastTestToResultMs_;
  snapshot.passReadyToUnlockMs = lastPassReadyToUnlockMs_;
  snapshot.startToUnlockMs = lastStartToUnlockMs_;
  snapshot.consecutiveFailCount = consecutiveFailCount_;
  snapshot.retestRequired = state_ == SystemState::RetestRequired || state_ == SystemState::RetestSampling;
  snapshot.retestIntervalMs = config::timing::kRetestIntervalMs;
  snapshot.retestCycleCount = retestCycleCount_;
  snapshot.retestDueToTestMs = lastRetestDueToTestMs_;
  snapshot.retestToResultMs = lastRetestToResultMs_;

  if ((state_ == SystemState::Running || state_ == SystemState::RetestRequired ||
       state_ == SystemState::RetestSampling || samplingWhileRunning_) &&
      runningSessionStartedAtMs_ > 0 && nowMs >= runningSessionStartedAtMs_) {
    snapshot.runningSessionMs = nowMs - runningSessionStartedAtMs_;
  }

  if (state_ == SystemState::Running && nextRetestAtMs_ > 0) {
    snapshot.retestRemainingMs = nowMs < nextRetestAtMs_ ? (nextRetestAtMs_ - nowMs) : 0;
  } else if (state_ == SystemState::RetestRequired) {
    snapshot.retestRemainingMs = 0;
    snapshot.retestOverdueMs =
        retestRequiredAtMs_ > 0 && nowMs >= retestRequiredAtMs_ ? (nowMs - retestRequiredAtMs_) : 0;
    snapshot.retestGraceRemainingMs = snapshot.retestOverdueMs < config::timing::kRetestGraceMs
                                          ? (config::timing::kRetestGraceMs - snapshot.retestOverdueMs)
                                          : 0;
  } else if ((state_ == SystemState::RetestSampling || samplingWhileRunning_) && retestRequiredAtMs_ > 0 &&
             nowMs >= retestRequiredAtMs_) {
    snapshot.retestOverdueMs = nowMs - retestRequiredAtMs_;
  }

  if (state_ == SystemState::Preheat && nowMs >= preheatStartedAtMs_ &&
      nowMs - preheatStartedAtMs_ < config::timing::kPreheatMs) {
    snapshot.preheatRemainingMs = config::timing::kPreheatMs - (nowMs - preheatStartedAtMs_);
  }

  snapshot.sampling.active = sampling_.active;
  snapshot.sampling.collected = sampling_.collected;
  snapshot.sampling.total = config::timing::kSampleCount;
  snapshot.sampling.startedAtMs = sampling_.startedAtMs;
  snapshot.sampling.completedAtMs = sampling_.completedAtMs;
  snapshot.sampling.durationMs = sampling_.active ? (nowMs - sampling_.startedAtMs) : lastTestToResultMs_;
  snapshot.sampling.latestRaw = sampling_.lastRaw;
  snapshot.sampling.stdDev = sampledStdDev_;
  return snapshot;
}

}  // namespace app
