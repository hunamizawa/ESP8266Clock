#include "Brightness.h"
#include <Arduino.h>

// #define DECREASING_THAN(th) (b > static_cast<int32_t>(th) - _setting.hysteresis && v <= static_cast<int32_t>(th) - _setting.hysteresis)
// #define INCREASING_THAN(th) (b < static_cast<int32_t>(th) + _setting.hysteresis && v >= static_cast<int32_t>(th) + _setting.hysteresis)

bool Brightness::decrasingThan(TBufElem b, TBufElem v, TBufElem th) const {
  auto true_th = th - _setting.hysteresis;
  return b > true_th && v <= true_th;
}

bool Brightness::incrasingThan(TBufElem b, TBufElem v, TBufElem th) const {
  auto true_th = th + _setting.hysteresis;
  return b < true_th && v >= true_th;
}

int8_t Brightness::calcBrightness(TBufElem v) {
  auto b = static_cast<TBufElem>(_before);

  if (incrasingThan(b, v, _setting.thresholds.at(0)))
    _current = -1;
  else if (incrasingThan(b, v, _setting.thresholds.at(1)) || decrasingThan(b, v, _setting.thresholds.at(0)))
    _current = 0;
  else if (incrasingThan(b, v, _setting.thresholds.at(2)) || decrasingThan(b, v, _setting.thresholds.at(1)))
    _current = 1;
  else if (incrasingThan(b, v, _setting.thresholds.at(3)) || decrasingThan(b, v, _setting.thresholds.at(2)))
    _current = 3;
  else if (incrasingThan(b, v, _setting.thresholds.at(4)) || decrasingThan(b, v, _setting.thresholds.at(3)))
    _current = 6;
  else if (incrasingThan(b, v, _setting.thresholds.at(5)) || decrasingThan(b, v, _setting.thresholds.at(4)))
    _current = 10;
  else if (decrasingThan(b, v, _setting.thresholds.at(5)))
    _current = 15;

  return _current;
}

// #undef DECREASING_THAN
// #undef INCREASING_THAN

int8_t Brightness::update(uint16_t v) {
  if (_buffer.empty())
    _buffer.assign(_buffer.capacity(), v); // buffer が常に満杯であることを保証する
  else
    _buffer.push_back(v);

  auto avg = calcAverageRawValue();
  _before  = v;
  return calcBrightness(avg);
}

void Brightness::changeSetting(const brightness_setting_t &setting) {
  _setting = setting;
}

int8_t Brightness::getValue() const {
  return _current;
}

uint16_t Brightness::calcAverageRawValue() const {
  return std::accumulate(_buffer.begin(), _buffer.end(), (TBufElem)0) / _buffer.size();
}
