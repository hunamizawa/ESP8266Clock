/**
 * @file MyBuffer.h
 */

#ifndef MYBUFFER_H_
#define MYBUFFER_H_

#include "myutil.h"
#include "../envdata_t.h"
#include "MyGraphics.h"
#include "Panes.h"
#include <IPAddress.h>
#include <MAX7219Display.h>
#include <string>
using std::u16string;

class MyBuffer : public MAX7219::Buffer<32, 16, MyGraphics> {
private:
  Panes         _pane;
  Panes         _prev_pane;
  OverridePanes _override_pane;
  OverridePanes _prev_override_pane;
  struct tm     _prev_tm;
  suseconds_t   _prev_us;
  envdata_t     _prev_envData;
  IPAddress     _prev_addr = IPAddress();

  /**
   * @brief 「時刻＋他の情報」を表示するために、時刻を 2 行目に描画する
   * 
   * @param tm 現在時刻
   */
  void timeRow2(const struct tm &tm);

  /**
   * @brief _prev_** で始まるメンバ変数を現在の値で上書きする
   * 
   * @param tm 現在時刻
   * @param us 現在時刻のマイクロ秒部分
   * @param envData 最新の環境計測結果
   * @param addr 現在の IP アドレス
   */
  void assignPrevValues(const struct tm &tm, suseconds_t us, const envdata_t &envData, IPAddress *addr);

public:
  MyBuffer()
      : MAX7219::Buffer<32, 16, MyGraphics>::Buffer() {}
  DISALLOW_COPY(MyBuffer);
  ALLOW_DEFAULT_MOVE(MyBuffer);

  /**
   * @brief 画面の更新が必要か判定する
   * 
   * @param tm 現在時刻
   * @param us 現在時刻のマイクロ秒部分
   * @param envData 気温・湿度・気圧
   * @param addr IP アドレス
   * @retval true update() を呼び出してください
   * @retval false 何もしなくていい
   * @post この関数が true を返したら、update() を呼び出すべき。
   */
  bool isRequireUpdate(const struct tm &tm, suseconds_t us, const envdata_t &envData, IPAddress *addr) const;
  /**
   * @brief 画面を更新する
   * 
   * @param tm 現在時刻
   * @param us 現在時刻のマイクロ秒部分
   * @param envData 気温・湿度・気圧
   * @param addr IP アドレス
   * @note MAX7219::Display::send() を呼び出すのを忘れないように。
   */
  void update(const struct tm &tm, suseconds_t us, const envdata_t &envData, IPAddress *addr);

  void  setPane(const Panes pane);
  Panes getPane() const;

  void          setOverridePane(const OverridePanes pane);
  OverridePanes getOverridePane() const;
};

#endif // MYBUFFER_H_