/**
 * @file IBuffer.h
 */

#ifndef MAX7219Display_IBuffer_H_
#define MAX7219Display_IBuffer_H_

#include <Arduino.h>

namespace MAX7219 {

/**
 * @brief Buffer に対する読み取り専用操作を提供するインターフェイス
 * 
 * Display からはこのインターフェイスを扱い、Buffer を直接触らない。
 */
class IBuffer {
public:
  /**
   * @brief 座標 (x,y) から x 軸方向に 8 ビット分のグラフィックを取得
   * 
   * @param x 始点の x 座標
   * @param y 始点の y 座標
   * @return 8 ビット分のグラフィックを表す値
   */
  virtual const uint8_t getHorizontialFrom(size_t x, size_t y, bool swap) const = 0;

  /**
   * @brief 座標 (x,y) から y 軸方向に 8 ビット分のグラフィックを取得
   * 
   * @param x 始点の x 座標
   * @param y 始点の y 座標
   * @return 8 ビット分のグラフィックを表す値
   */
  virtual const uint8_t getVerticalFrom(size_t x, size_t y, bool swap) const = 0;

  /**
   * @brief バッファの幅を取得
   */
  virtual const uint8_t getBufferWidth() const  = 0;

  /**
   * @brief バッファの高さを取得
   */
  virtual const uint8_t getBufferHeight() const = 0;

  /**
   * @brief 現在のバッファの内容を Serial へ出力する（デバッグ用）
   */
  virtual void printToSerial() const = 0;
};

} // namespace MAX7219

#endif // MAX7219Display_IBuffer_H_