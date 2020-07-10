#include "Brightness.h"
#include <Arduino.h>

bool Brightness::decrasingThan(TBufElem b, TBufElem v, size_t threshold_index) const {
  auto true_th = _setting.thresholds.at(threshold_index) - _setting.hysteresis;
  return b > true_th && v <= true_th;
}

bool Brightness::incrasingThan(TBufElem b, TBufElem v, size_t threshold_index) const {
  auto true_th = _setting.thresholds.at(threshold_index) + _setting.hysteresis;
  return b < true_th && v >= true_th;
}

int8_t Brightness::calcBrightness(TBufElem v) {
  auto b = static_cast<TBufElem>(_before);

  if (incrasingThan(b, v, 0))
    _current = -1;
  else if (incrasingThan(b, v, 1) || decrasingThan(b, v, 0))
    _current = 0;
  else if (incrasingThan(b, v, 2) || decrasingThan(b, v, 1))
    _current = 1;
  else if (incrasingThan(b, v, 3) || decrasingThan(b, v, 2))
    _current = 2;
  else if (incrasingThan(b, v, 4) || decrasingThan(b, v, 3))
    _current = 3;
  else if (incrasingThan(b, v, 5) || decrasingThan(b, v, 4))
    _current = 4;
  else if (decrasingThan(b, v, 5))
    _current = 5;

  return _current;
}

int8_t Brightness::update(uint16_t v) {
  if (_buffer.empty())
    _buffer.assign(_buffer.capacity(), v); // buffer が常に満杯であることを保証する
  else
    _buffer.push_back(v);

  auto avg    = calcAverageRawValue();
  auto retval = calcBrightness(avg);
  _before     = avg;
  return retval;
}

void Brightness::changeSetting(const brightness_setting_t &setting) {
  _setting = setting;
}

int8_t Brightness::getBrightness() const {
  return _current;
}

uint16_t Brightness::calcAverageRawValue() const {
  return std::accumulate(_buffer.begin(), _buffer.end(), (TBufElem)0) / _buffer.size();
}
