#pragma once

#include "io_devices.h"

namespace app {

class OledUi {
 public:
  void render(IoDevices& io, const AppSnapshot& snapshot);

 private:
  void drawLiveLine(Adafruit_SSD1306& display, int y, const char* label, uint16_t value);
};

}  // namespace app
