#ifndef envdata_t_H_
#define envdata_t_H_

#include <Arduino.h>

/**
 * @brief 気温計測結果を表す struct
 */
typedef struct EnvData {
  time_t time;
  float  temperature;
  float  humidity;
  float  pressure;

  bool operator==(const struct EnvData &rhs) const {
    return time == rhs.time &&
           temperature == rhs.temperature &&
           humidity == rhs.humidity &&
           pressure == rhs.pressure;
  }

  bool operator!=(const struct EnvData &rhs) const{
    return !(*this == rhs);
  }
} envdata_t;

#endif // envdata_t_H_