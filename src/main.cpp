/**
 * @file main.cpp
 */

#include "main.h"
#include "TZDB.h"
#include "envdata_t.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <TZ.h>
#include <Wire.h>
#include <map>
#include <time.h>

ClockSetting _setting;

/**
 * @brief 時刻同期を実行する
 * 
 * @pre WiFi の接続が完了していること。
 */
void syncTime() {
  // タイムゾーン設定を読み取り
  PGM_P tz = TZDB::getTZ(_setting.tzarea, _setting.tzcity);
  if (!tz)
    tz = TZ_Etc_GMT;

  // NTPサーバー設定を読み取り
  std::array<const char *, 3> ntp = {nullptr, nullptr, nullptr};

  size_t i = 0;
  for (auto &&n : _setting.ntp) {
    if (i >= ntp.size())
      break;

    if (!n || n.isEmpty())
      continue;

    ntp.at(i) = n.c_str();
    ++i;
  }

  // 有効なNTPサーバーが指定されなければデフォルト値を使用
  if (i == 0)
    ntp.at(0) = DEFAULT_NTP_SERVER;

  // 同期開始
  pftime::configTime(tz, ntp.at(0), ntp.at(1), ntp.at(2));

  // 同期が完了するまで待つ
  while (pftime::time(nullptr) < 10000)
    yield();
}

/**
 * @brief 1秒に1回実行する処理を記述する
 * 
 * @param tm 現在時刻
 */
void runEverySeconds(const struct tm &tm) {
  // 特にない
}

/**
 * @brief 毎分 00 秒に実行する処理を記述する
 * 
 * @param tm 現在時刻
 */
void runEveryMinutes(const struct tm &tm) {
  // 特にない
}

/**
 * @brief 毎時 00 分 00 秒に実行する処理を記述する
 * 
 * @param tm 現在時刻
 */
void runEveryHours(const struct tm &tm) {
  // 特にない
}

/**
 * @brief リセット後1度だけ実行される関数
 */
void setup() {

  Serial.begin(SERIAL_BAUD_RATE);
  randomSeed(ESP.getChipId());

  // 電源投入時の電圧変動が落ち着くのと、Serial が確実に開くのを待つ
  delay(500);

  // SPI、ディスプレイ初期化
  displayAndBufferInit();
  startWelcomeDisplay();

  // 温度センサー初期化
  Wire.begin(I2C_SDA, I2C_SCK);
  bmeInit();

  connectWiFi();
  // WiFiManager による Wi-Fi 接続が成功すると、
  // WiFiManager が内部で持っている ESP8266WebServer は stop されるので
  // 別の Server を立ち上げても問題なくなる

  // FS が壊れていても、デフォルト値を使えば時計としての機能は損なわれない
  assert_debug(SPIFFS.begin());
  //_datas.init();
  readSetting();
  setupServer();
  syncTime();

  auto tm = *pftime::localtime(nullptr, nullptr);

  runEverySeconds(tm);
  runEveryMinutes(tm);
  runEveryHours(tm);

  // 気温計測開始コマンド送信
  startMeasureEnvironment(tm);
  // 計測完了を待つ <= 9.3 ms
  delay(10);
  // 計測結果読み取り
  readEnvironment();

  stopWelcomeDisplay();
  _buffer.setPane(_setting.pane);
}

/**
 * @brief @c x [us] を 10 ms (10000 us) 単位で切り上げる
 * 
 * @param x マイクロ秒
 * @return <code>x @<= y && y % 10000 == 0</code> を満たすような最小の整数 @c y
 */
static inline suseconds_t ceiling10ms(suseconds_t x) {
  return ((x / 10000) + 1) * 10000;
}

/**
 * @brief ヒマな時に呼び出し続けないといけない関数
 */
void keeping() {
  yieldServer();
  yield();
}

/**
 * @brief target で指示された時刻になったかな？
 * 
 * @param target 
 * @retval true まだ
 * @retval false なった
 */
bool timeBefore(suseconds_t target) {

  static suseconds_t before = 0;
  static suseconds_t current;

  pftime::localtime(nullptr, &current);

  auto retval = current < target && current > before;
  before      = current;

  return retval;
}

/**
 * @brief メインループ
 * 
 */
void loop() {

  static suseconds_t usec;
  static struct tm   tm;
  static uint8_t     brightness_count = 0;

  // 現在時刻
  auto new_tm = *pftime::localtime(nullptr, &usec);

  changePaneIfSELPushed();

  updateDisplay(new_tm, usec);

  if (new_tm != tm) {
    tm = new_tm;

    // この節は1秒に1回だけ実行される

    runEverySeconds(tm);

    if (tm.tm_sec == 0) {
      runEveryMinutes(tm);
      if (tm.tm_min == 0)
        runEveryHours(tm);
    }

    if (tm.tm_sec == 0 || tm.tm_sec == 30) {
      if (_last_envdata.time == 0)
        bmeInit();
      // 毎分 00 秒、30 秒に気温計測開始コマンドを送る
      startMeasureEnvironment(tm);
    } else if (tm.tm_sec == 1 || tm.tm_sec == 31) {
      // 毎分 01 秒、31 秒に計測結果を読み取る
      readEnvironment();
    }

    // 重い処理なのでなるべくループの最後に実行する
    if (tm.tm_sec == 3 && tm.tm_min % DATA_SEND_INTERVAL == 0 && !_datas.empty()) {

      size_t ambient_transfered, custom_server_transfered;
      auto   enable_ambient       = _setting.use_ambient && _setting.ambient_channelid != 0 && _setting.ambient_writekey;
      auto   enable_custom_server = _setting.use_custom_server && _setting.custom_server_addr;

      if (enable_ambient) {
        auto ambient_addr  = "http://ambidata.io/api/v2/channels/" + String(_setting.ambient_channelid) + "/dataarray";
        ambient_transfered = postEnvdatas(ambient_addr, _setting.ambient_writekey, DATA_SEND_MAXCOUNT);

        if (!enable_custom_server) {
          for (size_t i = 0; i < ambient_transfered; i++)
            _datas.pop_front();
        }
      }

      if (enable_custom_server) {
        if (enable_ambient)
          yield();

        custom_server_transfered = postEnvdatas(_setting.custom_server_addr, _setting.custom_server_writekey, DATA_SEND_MAXCOUNT);

        if (!enable_ambient) {
          for (size_t i = 0; i < custom_server_transfered; i++)
            _datas.pop_front();
        }
      }

      if (enable_ambient && enable_custom_server && ambient_transfered == custom_server_transfered) {
        for (size_t i = 0; i < ambient_transfered; i++)
          _datas.pop_front();
      }
    }
  }

  // 約 40 ms ごとに測光する
  if (++brightness_count % 4 == 0)
    readAndSetBrightness();

  // loop() が大体 10 ms ごとに呼ばれるように調節する
  // 現在時刻をもう一度取得
  pftime::localtime(nullptr, &usec);
  // 次に来るキリのいい時間を計算
  auto target = ceiling10ms(usec);
  // target を過ぎるまで待つ
  while (timeBefore(target)) {
    keeping();
  }
}
