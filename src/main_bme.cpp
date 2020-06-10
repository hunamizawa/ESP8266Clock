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
 */
bool bmeInit() {
  _bme280.setI2CAddress(0x77);
  if (_bme280.beginI2C())
    return true;

  _bme280.setI2CAddress(0x76);
  return _bme280.beginI2C();
}

/**
 * @brief 計測開始コマンドをセンサに送る
 * 
 * @param tm 現在時刻（この時刻が envdata_t::time に設定される）
 * @pre bmeInit() を少なくとも 1 回呼び出す必要がある
 * @post 計測は非同期に行われるので、 readEnvironment() を呼ぶ前に完了待ちが必要 (9.3 ms max.)
 */
void startMeasureEnvironment(const struct tm &tm) {

  _measure_start = tm;
  // 計測開始
  _bme280.setMode(MODE_FORCED);
}

/**
 * @brief 計測結果をセンサから読み出す
 * 
 * @pre startMeasureEnvironment() を先に呼び出す必要がある
 * @note 計測結果は _last_envdata および _datas に反映される
 */
void readEnvironment() {

  if (_measure_start == (struct tm){0})
    return;

  auto temperature = _bme280.readTempC();
  auto humidity    = _bme280.readFloatHumidity();
  auto pressure    = _bme280.readFloatPressure() / 100.0f;

  // 通信に失敗した場合、各値に変な値が入っていることがあるので
  // Operating range 外の値があれば欠測にする
  if (temperature < -40.0f || temperature > 85.0f || humidity < 0.0f || humidity > 100.0f || pressure < 300.0f || pressure > 1100.0f) {

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
    if (_measure_start.tm_sec == 0 && data.isValid())
      _datas.push_back(data);
  }

  _measure_start = {0};
}