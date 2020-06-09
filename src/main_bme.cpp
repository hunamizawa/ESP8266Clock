/**
 * @file main_bme.cpp
 * @brief part of the main.cpp
 */

#include "main.h"
#include <SparkFunBME280.h>

//! BME280 環境計測センサ
static BME280 _bme280;
//! Ambient への送信のため、過去の環境計測結果を貯めておくキュー
EnvDataQueue _datas(ENVDATA_STOCK_MAX);
//! 直前の環境計測結果
envdata_t _last_envdata = {0};

static struct tm _measure_start = {0};

/**
 * @brief BME280 を初期化する
 * 
 */
bool bmeInit() {
  _bme280.setI2CAddress(0x77);
  if (_bme280.beginI2C())
    return true;

  _bme280.setI2CAddress(0x76);
  return _bme280.beginI2C();
}

/**
 * @brief 気温等を計測し、_datas に push
 * 
 * @param tm 現在時刻
 */
void startMeasureEnvironment(const struct tm &tm) {

  // 計測開始
  _bme280.setMode(MODE_FORCED);

  // 完了待ちが必要 (T_measure <= 9.3 ms)
}

void readEnvironment() {

  auto temperature = _bme280.readTempC();
  auto humidity    = _bme280.readFloatHumidity();
  auto pressure    = _bme280.readFloatPressure() / 100.0f;

  if (pressure == 0.0f) {

    _last_envdata = {0};

  } else {

    envdata_t data = {
        mktime(&_measure_start),
        temperature,
        humidity,
        pressure,
    };
    _last_envdata = data;

    // 毎分 00 秒に計測したデータだけを Ambient に送る
    if (_measure_start.tm_sec == 0)
      _datas.push_back(data);
  }

  _measure_start = {0};
}