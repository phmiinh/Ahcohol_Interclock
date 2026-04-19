#include "state_machine.h"

namespace app {

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

  if (fault_ == FaultCode::None && !refreshLiveAdc()) {
    updateUi(nowMs, true);
    return;
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
      updateSampling(nowMs);
      break;

    case SystemState::PassReady:
      if (startPressed) {
        telemetry_.logAction("START accepted. Vehicle unlocked");
        transitionTo(SystemState::Running);
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
      if (startPressed) {
        telemetry_.logAction("START pressed while RUNNING. Returning to locked standby");
        transitionTo(SystemState::StandbyLocked);
      }
      break;

    case SystemState::ErrorLocked:
      break;
  }

  updatePassBeep(nowMs);
  if (state_ == SystemState::FailLocked) {
    updateFailBuzzer(nowMs);
  } else if (!passBeep_.active) {
    io_.setBuzzer(false);
  }

  updateUi(nowMs);
}

void AlcoholInterlockController::transitionTo(SystemState newState) {
  state_ = newState;
  passBeep_ = {};

  if (state_ != SystemState::Sampling) {
    sampling_.active = false;
  }

  switch (state_) {
    case SystemState::Preheat:
    case SystemState::StandbyLocked:
    case SystemState::Sampling:
    case SystemState::PassReady:
      io_.setBuzzer(false);
      io_.lockVehicle();
      break;

    case SystemState::FailLocked:
      io_.setBuzzer(false);
      io_.lockVehicle();
      break;

    case SystemState::Running:
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
    passBeep_.startedAtMs = millis();
    passBeep_.durationMs = config::timing::kPassBeepMs;
    io_.setBuzzer(true);
  }

  const AppSnapshot current = snapshot(millis());
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

bool AlcoholInterlockController::refreshLiveAdc() {
  uint16_t raw = 0;
  if (!io_.readAlcoholRaw(raw)) {
    enterFault(FaultCode::AdcReadInvalid, "ADC returned invalid data. System locked");
    return false;
  }

  lastAlcoholRaw_ = raw;
  return true;
}

void AlcoholInterlockController::updatePreheat(uint32_t nowMs) {
  if (nowMs - preheatStartedAtMs_ >= config::timing::kPreheatMs) {
    transitionTo(SystemState::StandbyLocked);
  }
}

void AlcoholInterlockController::startSampling(uint32_t nowMs) {
  sampling_ = {};
  sampling_.active = true;
  sampling_.startedAtMs = nowMs;
  sampling_.nextSampleAtMs = nowMs;
  sampledAlcoholRaw_ = 0;
  transitionTo(SystemState::Sampling);
  telemetry_.logSamplingStarted(snapshot(nowMs));
}

void AlcoholInterlockController::updateSampling(uint32_t nowMs) {
  if (!sampling_.active) {
    return;
  }

  while (sampling_.active && sampling_.collected < config::timing::kSampleCount &&
         nowMs >= sampling_.nextSampleAtMs) {
    uint16_t raw = 0;
    if (!io_.readAlcoholRaw(raw)) {
      enterFault(FaultCode::AdcReadInvalid, "ADC returned invalid data during sampling");
      return;
    }

    sampling_.sum += raw;
    sampling_.lastRaw = raw;
    lastAlcoholRaw_ = raw;
    ++sampling_.collected;
    sampling_.nextSampleAtMs += config::timing::kSampleIntervalMs;

    telemetry_.logSamplingProgress(
        sampling_.collected,
        config::timing::kSampleCount,
        raw,
        nowMs - sampling_.startedAtMs);
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

  const bool passed = sampledAlcoholRaw_ < config::thresholds::kAlcoholAdc;
  telemetry_.logSamplingResult(snapshot(nowMs), nowMs - sampling_.startedAtMs, passed);

  transitionTo(passed ? SystemState::PassReady : SystemState::FailLocked);
  telemetry_.emitDashboardEvent(
      "sample_result",
      passed ? "success" : "warning",
      passed ? "Alcohol level safe" : "Alcohol level above threshold",
      snapshot(millis()));
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
  snapshot.uptimeMs = nowMs;

  if (state_ == SystemState::Preheat && nowMs >= preheatStartedAtMs_ &&
      nowMs - preheatStartedAtMs_ < config::timing::kPreheatMs) {
    snapshot.preheatRemainingMs = config::timing::kPreheatMs - (nowMs - preheatStartedAtMs_);
  }

  snapshot.sampling.active = sampling_.active;
  snapshot.sampling.collected = sampling_.collected;
  snapshot.sampling.total = config::timing::kSampleCount;
  snapshot.sampling.startedAtMs = sampling_.startedAtMs;
  snapshot.sampling.completedAtMs = sampling_.completedAtMs;
  snapshot.sampling.latestRaw = sampling_.lastRaw;
  return snapshot;
}

}  // namespace app
