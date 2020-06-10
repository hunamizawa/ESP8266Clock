/**
 * @file envdata_t.h
 */

#ifndef envdata_t_H_
#define envdata_t_H_

#include <Arduino.h>

/**
 * @brief 気温計測結果を表す構造体
 */
typedef struct EnvData {
  time_t time;        //! 観測時刻
  float  temperature; //! 気温 (℃)
  float  humidity;    //! 湿度 (%)
  float  pressure;    //! 気圧 (hPa)

  bool operator==(const struct EnvData &rhs) const {
    return time == rhs.time &&
           temperature == rhs.temperature &&
           humidity == rhs.humidity &&
           pressure == rhs.pressure;
  }

  bool operator!=(const struct EnvData &rhs) const {
    return !(*this == rhs);
  }

  /**
   * @brief envdata_t が有効な値かどうか調べる
   * 
   * @retval true envdata_t の値は有効
   * @retval false envdata_t の値は無効
   */
  bool isValid() const {
    return time != 0;
  }
} envdata_t;

#endif // envdata_t_H_