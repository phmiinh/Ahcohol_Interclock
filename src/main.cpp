#include <Arduino.h>

#include "state_machine.h"

namespace {
app::AlcoholInterlockController controller;
}

void setup() {
  controller.begin();
}

void loop() {
  controller.update();
}
