#include "Brightness.h"
#include <Arduino.h>

#define DECREASING_THAN(th) (b > (th) - _setting.hysteresis && v <= (th) - _setting.hysteresis)
#define INCREASING_THAN(th) (b < (th) + _setting.hysteresis && v >= (th) + _setting.hysteresis)

int8_t Brightness::calcBrightness(uint16_t v) {
  auto b = _before;

  if (INCREASING_THAN(_setting.thresholds.at(0))) {
    return _current = -1;
  } else if (INCREASING_THAN(_setting.thresholds.at(1)) || DECREASING_THAN(_setting.thresholds.at(0))) {
    return _current = 0;
  } else if (INCREASING_THAN(_setting.thresholds.at(2)) || DECREASING_THAN(_setting.thresholds.at(1))) {
    return _current = 1;
  } else if (INCREASING_THAN(_setting.thresholds.at(3)) || DECREASING_THAN(_setting.thresholds.at(2))) {
    return _current = 3;
  } else if (INCREASING_THAN(_setting.thresholds.at(4)) || DECREASING_THAN(_setting.thresholds.at(3))) {
    return _current = 6;
  } else if (INCREASING_THAN(_setting.thresholds.at(5)) || DECREASING_THAN(_setting.thresholds.at(4))) {
    return _current = 10;
  } else if (DECREASING_THAN(_setting.thresholds.at(5))) {
    return _current = 15;
  } else {
    return _current;
  }
}

#undef DECREASING_THAN
#undef INCREASING_THAN

int8_t Brightness::update(uint16_t v) {
  if (_buffer.empty())
    _buffer.assign(_buffer.capacity(), v); // buffer が常に満杯であることを保証する
  else
    _buffer.push_back(v);

  auto avg = std::accumulate(_buffer.begin(), _buffer.end(), 0) / _buffer.size();
  return calcBrightness(avg);
}

void Brightness::changeSetting(const brightness_setting_t &setting) {
  _setting = setting;
}

int8_t Brightness::getValue() {
  return _current;
}

uint16_t Brightness::getRawValue() {
  return _buffer.back();
}
