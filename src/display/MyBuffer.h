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

  void timeRow2(const struct tm &tm);

  /**
   * @brief _prev_** で始まるメンバ変数を現在の値で上書きする
   * 
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
   * @return true update() を呼び出してください
   * @return false 何もしなくていい
   */
  bool isRequireUpdate(const struct tm &tm, suseconds_t us, const envdata_t &envData, IPAddress *addr) const;
  /**
   * @brief 画面を更新する
   * 
   * @param tm 現在時刻
   * @param us 現在時刻のマイクロ秒部分
   * @param envData 気温・湿度・気圧
   * @param addr IP アドレス
   */
  void update(const struct tm &tm, suseconds_t us, const envdata_t &envData, IPAddress *addr);
  /**
   * @brief 画面を切り替える
   * 
   * @param pane 表示したい画面 
   */
  void  setPane(const Panes pane);
  Panes getPane() const;

  void          setOverridePane(const OverridePanes pane);
  OverridePanes getOverridePane() const;
};

#endif // MYBUFFER_H_