/*
==================================
MAX7219::BufferBase::_buffer の説明
==================================
class MAX7219::BufferBase は、LED モジュール用のフレームバッファです。
LED モジュールに表示させるグラフィックを、uint8_t[] 配列の形で管理します。

例えば MAX7219::BufferBase<3, 2> の場合、
_buffer[] はマトリックスLEDに対して、次のようにマッピングされます。

       x: 0 1 ...     7 8 9 ...    15 16 17 ...  23
         ┌─────────────┬─────────────┬─────────────┐
         │(MSB)                               (LSB)│
 0     0 │               _buffer[ 0]               │
       1 │               _buffer[ 1]               │
     ... │                   ...                   │
       7 │               _buffer[ 7]               │
         ├─────────────┼─────────────┼─────────────┤
 1     8 │               _buffer[ 8]               │
       9 │               _buffer[ 9]               │
     ... │                   ...                   │
      15 │               _buffer[15]               │
 ↑     ↑ └─────────────┴─────────────┴─────────────┘
 |     └ y
 └ device_y

uint8_t の各ビットが、マトリックスLEDの横1行（8ドット）に対応しています。
例えば 0x80 (== 0b10000000) なら、一番左のドットだけ点灯して、他は消灯します。
uint8_t を縦方向に積み重ねることで、LEDモジュール全体を表現しています。

_buffer に加えた変更は、MAX7219::Display::send() を呼び出すことにより
SPI 経由で MAX7219 に送られ、LEDモジュールの表示が更新されます。
*/

#ifndef MAX7219Display_BufferBase_H_
#define MAX7219Display_BufferBase_H_

#include "bitset.h"
#include "IBuffer.h"
#include <array>
#include <stdint.h>

namespace MAX7219 {

/**
 * @brief MAX7219 ディスプレイに表示するグラフィックを保持し、グラフィックの書き換え・消去といった基本的な機能を提供するクラス
 * 
 * @tparam BufferWidth バッファ領域の幅
 * @tparam BufferHeight バッファ領域の高さ
 */
template <size_t BufferWidth, size_t BufferHeight>
class BufferBase : public IBuffer {
  // TODO: 毎回バッファ全部を送るのはやめて、変更された部分だけ送るようにして
  // SPI の転送に要する時間を短縮したい
  // そのための _changed_area

private:
  using TBitset = MAX7219::bitset<BufferWidth>;
  using TBuffer = std::array<TBitset, BufferHeight>;

  TBuffer _buffer; // グラフィックを保持している配列（VRAMのようなもの）
  //TBuffer _changed_area;        // 更新された範囲を追跡するための配列

  /**
   * @brief 対象範囲が描画可能領域に含まれるかチェック
   * 
   * @param x 対象範囲の左上隅の x 座標
   * @param y 対象範囲の左上隅の y 座標
   * @param width 対象範囲の幅
   * @param height 対象範囲の高さ
   * @return true 対象範囲は描画可能領域に含まれない（描画処理の必要がない）
   * @return false 対象範囲は描画可能領域に含まれる
   */
  bool isOutOfBound(ssize_t x, ssize_t y, size_t width, size_t height) const {
    return x >= BufferWidth ||
           y >= BufferHeight ||
           x + width <= 0 ||
           y + height <= 0;
  }

  /**
   * @brief 対象座標が描画可能領域に含まれるかチェック
   * 
   * @param x 対象範囲の左上隅の x 座標
   * @param y 対象範囲の左上隅の y 座標
   * @return true 対象座標は描画可能領域に含まれない（描画処理の必要がない）
   * @return false 対象座標は描画可能領域に含まれる
   */
  bool isOutOfBound(ssize_t x, ssize_t y) const {
    return x >= BufferWidth ||
           y >= BufferHeight ||
           x < 0 ||
           y < 0;
  }

  bool isOutOfBound(size_t x, size_t y) const {
    return x >= BufferWidth ||
           y >= BufferHeight;
  }

public:
  // Disallow copy
  BufferBase(const BufferBase &) = delete;
  BufferBase &operator=(const BufferBase &) = delete;
  // Allow default move
  BufferBase(BufferBase &&) = default;
  BufferBase &operator=(BufferBase &&) = default;

  BufferBase() {
    _buffer = TBuffer();
    //_changed_area = TBuffer();
  }

  virtual ~BufferBase() = default;

  /**
   * @brief T[] 型で表されたグラフィックを指定位置に描画する
   * 
   * @tparam Tdata 
   * @param data グラフィックデータ（画像）
   * @param x 描画先の左上隅の x 座標
   * @param y 描画先の左上隅の y 座標
   * @param width グラフィックの有効幅
   * @param height グラフィックの高さ = data の要素数
   */
  template <class Tdata>
  void write(const Tdata *data, ssize_t x, ssize_t y, size_t width, size_t height) {
    if (!data)
      return;

    width = std::min(width, sizeof(Tdata) * 8U);

    if (isOutOfBound(x, y, width, height))
      return;

    size_t actual_width = width + std::min(x, 0);
    size_t right_space  = BufferWidth - x - actual_width;

    size_t buf_i;
    for (size_t data_i = -std::min(y, 0); data_i < height && (buf_i = y + data_i) < _buffer.size(); data_i++) {
      auto d = TBitset(data[data_i]) << right_space;
      //Serial.println(d.to_string(' ', '*').c_str());
      _buffer.at(buf_i).setRange(right_space, actual_width, false);
      _buffer.at(buf_i) |= d;
    }
  }

  /**
   * @brief std::array<T, Height> 型で表されたグラフィックを指定位置に描画する
   * 
   * @tparam Tdata 
   * @param data グラフィックデータ（画像）
   * @param x 描画先の左上隅の x 座標
   * @param y 描画先の左上隅の y 座標
   * @param width グラフィックの有効幅
   */
  template <class Tdata, size_t Height>
  void write(const std::array<Tdata, Height> &data, ssize_t x, ssize_t y, size_t width) {
    write(data.data(), x, y, width, data.size());
  }

  /**
   * @brief 指定ドットの ON/OFF を切り替える
   * 
   * @param is_on ON なら true
   * @param x 対象ドットの x 座標
   * @param y 対象ドットの y 座標
   */
  void turnDot(bool is_on, size_t x, size_t y) {
    size_t bit_i          = BufferWidth - x - 1;
    size_t buf_i          = y;
    _buffer.at(buf_i)[bit_i] = is_on;
  }

  /**
   * @brief 指定領域をクリア
   * 
   * @param x 領域の左上隅の x 座標
   * @param y 領域の左上隅の y 座標
   * @param width 領域の幅
   * @param height 領域の高さ
   */
  void clear(ssize_t x, ssize_t y, size_t width, size_t height) {
    if (isOutOfBound(x, y, width, height))
      return;

    if (x < 0) {
      width += x;
      x = 0;
    }
    if (y < 0) {
      height += y;
      y = 0;
    }

    size_t actual_width = width;
    size_t right_space  = BufferWidth - x - actual_width;

    size_t buf_i;
    for (size_t data_i = -std::min(y, 0); data_i < height && (buf_i = y + data_i) < _buffer.size(); data_i++) {
      _buffer.at(buf_i).setRange(right_space, actual_width, false);
    }
  }

  /**
   * @brief 全領域をクリア
   */
  void clearAll() {
    for (size_t i = 0; i < _buffer.size(); i++) {
      _buffer.at(i).reset();
      //_changed_area[i].set();
    }
  }

  /**
   * @brief 座標 (x,y) から x 軸方向に 8 ビット分のグラフィックを取得
   * 
   * @param x 
   * @param y 
   * @return const uint8_t 
   */
  virtual const uint8_t getHorizontialFrom(size_t x, size_t y, bool swap) const {
    if (isOutOfBound(x, y))
      return 0;

    auto retval = _buffer.at(y).template range<8>(x);
    if (!swap)
      retval.swap();
    return static_cast<uint8_t>(retval.to_ulong());
  }

  /**
   * @brief 座標 (x,y) から y 軸方向に 8 ビット分のグラフィックを取得
   * 
   * @param x 
   * @param y 
   * @return const uint8_t 
   */
  virtual const uint8_t getVerticalFrom(size_t x, size_t y, bool swap) const {
    if (isOutOfBound(x, y))
      return 0;

    auto   retval   = MAX7219::bitset<8>();
    size_t actual_x = BufferWidth - x - 1;

    if (swap) {
      for (size_t i = -std::min(y, 0U); i < 8 && i < _buffer.size(); i++)
        retval[i] = _buffer.at(y)[actual_x];
    } else {
      for (size_t i = -std::min(y, 0U); i < 8 && i < _buffer.size(); i++)
        retval[7 - i] = _buffer.at(y)[actual_x];
    }
    return static_cast<uint8_t>(retval.to_ulong());
  }

  virtual const uint8_t getBufferWidth() const {
    return BufferWidth;
  }

  virtual const uint8_t getBufferHeight() const {
    return BufferHeight;
  }

  /**
   * @brief 現在のバッファの内容を Serial へ出力する。
   * 
   */
  void printToSerial() const {
    Serial.println(std::string(BufferWidth, '-').c_str());
    for (size_t i = 0; i < _buffer.size(); i++) {
      auto bstr = _buffer.at(i).to_string(' ', '*');
      Serial.println(bstr.c_str());
    }
  }
};

}; // namespace MAX7219

#endif // MAX7219Display_BufferBase_H_