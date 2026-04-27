#include "ui_oled.h"

namespace app {

void OledUi::render(IoDevices& io, const AppSnapshot& snapshot) {
  if (!io.isDisplayReady()) {
    return;
  }

  Adafruit_SSD1306& display = io.display();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Alcohol Interlock");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  switch (snapshot.state) {
    case SystemState::Preheat:
      display.setCursor(0, 16);
      display.println("Sensor warm-up");
      display.setCursor(0, 28);
      display.print("Remain: ");
      display.print((snapshot.preheatRemainingMs + 999UL) / 1000UL);
      display.println("s");
      drawLiveLine(display, 40, "Live ADC: ", snapshot.liveAdc);
      display.setCursor(0, 52);
      display.println("Vehicle locked");
      break;

    case SystemState::StandbyLocked:
      display.setCursor(0, 16);
      display.println("Ready for TEST");
      drawLiveLine(display, 28, "Live ADC: ", snapshot.liveAdc);
      display.setCursor(0, 40);
      display.print("Threshold: ");
      display.println(snapshot.threshold);
      display.setCursor(0, 52);
      display.println("Press TEST");
      break;

    case SystemState::Sampling:
      display.setCursor(0, 16);
      display.println("Sampling...");
      display.setCursor(0, 28);
      display.print("Count: ");
      display.print(snapshot.sampling.collected);
      display.print("/");
      display.println(snapshot.sampling.total);
      drawLiveLine(display, 40, "Latest ADC: ", snapshot.sampling.latestRaw);
      display.setCursor(0, 52);
      display.print("Elapsed: ");
      display.print(snapshot.uptimeMs - snapshot.sampling.startedAtMs);
      display.println("ms");
      break;

    case SystemState::PassReady:
      display.setCursor(0, 16);
      display.println("PASS - SAFE");
      display.setCursor(0, 28);
      display.print("Average: ");
      display.println(snapshot.sampledAdc);
      drawLiveLine(display, 40, "Live ADC: ", snapshot.liveAdc);
      display.setCursor(0, 52);
      display.println("Press START");
      break;

    case SystemState::FailLocked:
      display.setCursor(0, 16);
      display.println("FAIL - LOCKED");
      display.setCursor(0, 28);
      display.print("Average: ");
      display.println(snapshot.sampledAdc);
      drawLiveLine(display, 40, "Live ADC: ", snapshot.liveAdc);
      display.setCursor(0, 52);
      display.println("Adjust then TEST");
      break;

    case SystemState::Running:
      display.setCursor(0, 16);
      display.println("RUNNING");
      display.setCursor(0, 28);
      display.println("Servo unlocked");
      drawLiveLine(display, 40, "Live ADC: ", snapshot.liveAdc);
      display.setCursor(0, 52);
      display.print("Retest in ");
      display.print((snapshot.retestRemainingMs + 999UL) / 1000UL);
      display.println("s");
      break;

    case SystemState::RetestRequired:
      display.setCursor(0, 16);
      display.println("RETEST REQUIRED");
      display.setCursor(0, 28);
      display.println("Press TEST again");
      drawLiveLine(display, 40, "Live ADC: ", snapshot.liveAdc);
      display.setCursor(0, 52);
      display.print("Timeout ");
      display.print((snapshot.retestGraceRemainingMs + 999UL) / 1000UL);
      display.println("s");
      break;

    case SystemState::RetestSampling:
      display.setCursor(0, 16);
      display.println("RUNNING RETEST");
      display.setCursor(0, 28);
      display.print("Count: ");
      display.print(snapshot.sampling.collected);
      display.print("/");
      display.println(snapshot.sampling.total);
      drawLiveLine(display, 40, "Latest ADC: ", snapshot.sampling.latestRaw);
      display.setCursor(0, 52);
      display.println("Vehicle still run");
      break;

    case SystemState::ErrorLocked:
      display.setCursor(0, 16);
      display.println("SYSTEM ERROR");
      display.setCursor(0, 28);
      display.print("Fault: ");
      display.println(faultToString(snapshot.fault));
      display.setCursor(0, 40);
      display.println("Safe state active");
      display.setCursor(0, 52);
      display.println(snapshot.fault == FaultCode::RetestTimeout ? "Press TEST retry" : "Vehicle locked");
      break;
  }

  display.display();
}

void OledUi::drawLiveLine(Adafruit_SSD1306& display, int y, const char* label, uint16_t value) {
  display.setCursor(0, y);
  display.print(label);
  display.print(value);
}

}  // namespace app
