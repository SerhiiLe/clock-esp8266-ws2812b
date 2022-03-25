#include "timerMinim.h"

timerMinim::timerMinim(uint32_t interval) {
  _interval = interval;
  _timer = millis();
}

void timerMinim::setInterval(uint32_t interval) {
  _interval = interval;
}

boolean timerMinim::isReady() {
  if (millis() - _timer >= _interval) {
    _timer = millis();
    return true;
  } else {
    return false;
  }
}

void timerMinim::reset() {
  _timer = millis();
}