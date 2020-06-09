/**
 * @file main_display.cpp
 * @brief part of the main.cpp
 */

#include "main.h"
#include <SPI.h>
#include <Ticker.h>

MyBuffer         _buffer;
MAX7219::Display _display(SPI_CS_DISPLAY, _buffer, _buffer_settings);
Brightness       _bn;

//! loop() に到達するまでの間、画面更新を担うタイマー
static Ticker _timer_update_display_until_setup;
//! 電源投入2秒後に「同期中...」と表示するためのタイマー
static Ticker _timer_pane_change;

/**
 * @brief 必要があれば画面を更新する
 * 
 * @param tm 現在時刻
 * @param usec 現在時刻のマイクロ秒部分
 */
void updateDisplay(const struct tm &tm, suseconds_t usec) {
  static IPAddress ip;
  ip = WiFi.localIP();

  if (_buffer.isRequireUpdate(tm, usec, _last_envdata, &ip)) {
    _buffer.update(tm, usec, _last_envdata, &ip);
    _display.send();
  }
}

/**
 * @brief 周囲の明るさを測定して、必要なら画面の明るさを変える
 * 
 */
void readAndSetBrightness() {

  static int8_t brightness;

  auto av = analogRead(A0);
  auto b  = _bn.update(av);

  if (b < 0 && _buffer.getOverridePane() == OverridePanes::NORMAL) {
    // b == -1 のときは画面オフ
    _buffer.setOverridePane(OverridePanes::OFF);
    _display.setIntensity(0);

  } else if (brightness != b) {
    if (_buffer.getOverridePane() == OverridePanes::OFF)
      _buffer.setOverridePane(OverridePanes::NORMAL);
    _display.setIntensity(b);
  }

  brightness = b;
}

/**
 * @brief SEL ボタンが押されていたら、画面をめくる
 * 
 */
void changePaneIfSELPushed() {

  // ボタンの以前の状態を保存する変数
  static bool clicked = false;

  auto c = digitalRead(PORT_SEL) == 0;

  if (!clicked && c) {
    // 画面をめくる処理
    switch (_buffer.getPane()) {
    case Panes::DATE_TIME:
      _setting.pane = Panes::TEMP_HUMI_TIME;
      break;

    case Panes::TEMP_HUMI_TIME:
      _setting.pane = Panes::PRES_TIME;
      break;

    case Panes::PRES_TIME:
      _setting.pane = Panes::TIME;
      break;

    case Panes::TIME:
      _setting.pane = Panes::IP_ADDR;
      break;

    case Panes::IP_ADDR:
      _setting.pane = Panes::DATE_TIME;
      break;

    default:
      break;
    }

    _buffer.setPane(_setting.pane);
  }

  clicked = c;
}

/**
 * @brief SPI、_buffer、_display を初期化
 * 
 */
void displayAndBufferInit() {

  // SPI 初期化
  pinMode(PORT_SEL, INPUT);
  SPI.begin();
  SPI.setFrequency(100000);
  SPI.setDataMode(SPI_MODE0);

  // ディスプレイ初期化
  _display.init();
  _buffer.setPane(Panes::WELCOME);
  _buffer.update({0}, 0, {0}, nullptr);
  _display.send();
  _display.shutdownMode(false);
}

/**
 * @brief ディスプレイ更新用タイマーを開始
 * 他の画面を表示させる前に必ず stopWelcomeDisplay() を呼び出すこと
 */
void startWelcomeDisplay() {

  _timer_update_display_until_setup.attach_ms(100, []() {
    suseconds_t usec;
    auto        tm = *pftime::localtime(nullptr, &usec);

    if (_buffer.isRequireUpdate(tm, usec, {0}, nullptr)) {
      _buffer.update(tm, usec, {0}, nullptr);
      _display.send();
    }
  });

  _timer_pane_change.once_ms(2000, []() {
    _buffer.setPane(Panes::SYNCING_TIME);
  });
}

/**
 * @brief ディスプレイ更新用タイマーを停止
 * 
 */
void stopWelcomeDisplay() {

  _timer_pane_change.detach();
  _timer_update_display_until_setup.detach();
}