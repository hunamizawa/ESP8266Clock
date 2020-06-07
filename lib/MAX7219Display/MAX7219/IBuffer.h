#ifndef MAX7219Display_IBuffer_H_
#define MAX7219Display_IBuffer_H_

#include <Arduino.h>

namespace MAX7219 {

/**
 * @brief Buffer に対する読み取り専用インターフェイス
 * MAX7219::Display からはこのインターフェイスしか触らない
 */
class IBuffer {
public:
  /**
   * @brief 座標 (x,y) から x 軸方向に 8 ビット分のグラフィックを取得
   * 
   * @param x 
   * @param y 
   * @return const uint8_t 
   */
  virtual const uint8_t getHorizontialFrom(size_t x, size_t y, bool swap) const = 0;

  /**
   * @brief 座標 (x,y) から y 軸方向に 8 ビット分のグラフィックを取得
   * 
   * @param x 
   * @param y 
   * @return const uint8_t 
   */
  virtual const uint8_t getVerticalFrom(size_t x, size_t y, bool swap) const = 0;

  virtual const uint8_t getBufferWidth() const  = 0;
  virtual const uint8_t getBufferHeight() const = 0;

  virtual void printToSerial() const = 0;
};

} // namespace MAX7219

#endif // MAX7219Display_IBuffer_H_