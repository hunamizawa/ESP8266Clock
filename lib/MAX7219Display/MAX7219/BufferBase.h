/**
 * @file BufferBase.h
 */

#ifndef MAX7219Display_BufferBase_H_
#define MAX7219Display_BufferBase_H_

#include "IBuffer.h"
#include "bitset.h"
#include <array>
#include <limits>
#include <stdint.h>

static constexpr ssize_t SSIZE_T_MAX = std::numeric_limits<ssize_t>::max();

namespace MAX7219 {

/**
 * @brief MAX7219 ディスプレイに表示するグラフィックを保持し、グラフィックの書き換え・消去といった基本的な機能を提供するクラス。
 * 
 * @tparam BufferWidth バッファ領域の幅
 * @tparam BufferHeight バッファ領域の高さ
 *
 * @par 内部実装
 * @parblock
 * MAX7219::BufferBase は、%MAX7219 LED モジュール用のフレームバッファです。
 * LED モジュールに表示させるグラフィックを、std::array<std::bitset<BufferWidth>, BufferHeight> の形で内部で保持しています。
 * 
 * 例えば MAX7219::BufferBase<24, 16> の場合、
 * 内部配列 @c _buffer は、マトリックス LED に対して次のようにマッピングされます。
 * 
 * @verbatim
 *        x: 0 1 ...     7 8 9 ...    15 16 17 ...  23
 *          ┌─────────────┬─────────────┬─────────────┐
 *          │[23](MSB)                        [0](LSB)│
 *  0     0 │               _buffer[ 0]               │
 *        1 │               _buffer[ 1]               │
 *      ... │                   ...                   │
 *        7 │               _buffer[ 7]               │
 *          ├─────────────┼─────────────┼─────────────┤
 *  1     8 │               _buffer[ 8]               │
 *        9 │               _buffer[ 9]               │
 *      ... │                   ...                   │
 *       15 │               _buffer[15]               │
 *  ↑     ↑ └─────────────┴─────────────┴─────────────┘
 *  |     └ y
 *  └ device_y
 * @endverbatim
 * 
 * ※bitset の 0 番目の要素は右端に来ることに注意して下さい。
 * 
 * すなわち、座標 (x, y) の ON/OFF は、
 * <tt>_buffer[y][BufferWidth - x - 1]</tt> にアクセスすることで取得・設定できます。
 * 例えば上図において (3, 5) を ON にするなら
 * @code
 *   _buffer[5][24 - 3 - 1] = true;
 * @endcode
 * とします。
 * 
 * @c _buffer に加えた変更は、Display::send() を呼び出すことにより
 * SPI 経由で %MAX7219 に送られ、LED モジュールの表示が更新されます。
 * @endparblock
 */
template <size_t BufferWidth, size_t BufferHeight>
class BufferBase : public IBuffer {
  // TODO: 毎回バッファ全部を送るのはやめて、変更された部分だけ送るようにして
  // SPI の転送に要する時間を短縮したい
  // そのための _changed_area

private:
  using TBitset = MAX7219::bitset<BufferWidth>;
  using TBuffer = std::array<TBitset, BufferHeight>;

  static constexpr ssize_t BufferWidth_S  = static_cast<ssize_t>(BufferWidth);
  static constexpr ssize_t BufferHeight_S = static_cast<ssize_t>(BufferHeight);

  TBuffer _buffer; //! グラフィックを保持している配列（VRAMのようなもの）
  //TBuffer _changed_area;        //! 更新された範囲を追跡するための配列

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

    return x >= BufferWidth_S ||
           y >= BufferHeight_S ||
           x + static_cast<ssize_t>(width) <= 0 ||
           y + static_cast<ssize_t>(height) <= 0;
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

    return x >= BufferWidth_S ||
           y >= BufferHeight_S ||
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
   * @brief @e Tdata [] 配列で表されたグラフィックを指定位置に描画する
   * 
   * @copydetails write(const std::array<Tdata, Height>&, ssize_t, ssize_t, size_t)
   * @param height グラフィックの高さ = @c data の要素数
   */
  template <class Tdata>
  void write(const Tdata *data, ssize_t x, ssize_t y, size_t width, size_t height) {

    // ssize_t にキャストしても値が壊れないことを保証
    assert(width <= SSIZE_T_MAX);
    assert(height <= SSIZE_T_MAX);

    if (!data)
      return;

    width = std::min(width, sizeof(Tdata) * 8U);

    if (isOutOfBound(x, y, width, height))
      return;

    auto   width_s      = static_cast<ssize_t>(width);
    size_t right_space  = BufferWidth_S - x - width_s; // グラフィックの右端と _buffer 右端の距離
    size_t actual_width = width_s + std::min(x, 0);

    size_t buf_i;
    for (size_t data_i = -std::min(y, 0); data_i < height && (buf_i = y + static_cast<ssize_t>(data_i)) < _buffer.size(); data_i++) {
      auto d = TBitset(data[data_i]) << right_space;
      _buffer.at(buf_i).setRange(right_space, actual_width, false);
      _buffer.at(buf_i) |= d;
    }
  }

  /**
   * @brief std::array<Tdata, Height> 型で表されたグラフィックを指定位置に描画する
   * 
   * @tparam Tdata 配列の型
   * @param data グラフィックデータ（画像）を表す配列
   * @param x 描画先の左上隅の x 座標
   * @param y 描画先の左上隅の y 座標
   * @param width グラフィックの有効幅
   * @pre <code>width @<= std::numeric_limits<ssize_t>::max()</code>（さもなければ assert failed）
   * @pre <code>height @<= std::numeric_limits<ssize_t>::max()</code>（さもなければ assert failed）
   * @note @e Tdata の型は <code>unsigned char</code> (= <code>uint8_t</code>), <code>unsigned int</code> (= <code>uint16_t</code>),
   *       <code>unsigned long</code> (= <code>uint32_t</code>), <code>unsigned long long</code> (= <code>uint64_t</code>) のいずれかであるべき。
   *       それ以外の型が @e Tdata に指定された場合の動作は未定義。
   */
  template <class Tdata, size_t Height>
  void write(const std::array<Tdata, Height> &data, ssize_t x, ssize_t y, size_t width) {
    write(data.data(), x, y, width, data.size());
  }

  /**
   * @brief 指定ドットの ON/OFF を切り替える
   * 
   * @param is_on ON にするなら true
   * @param x 対象ドットの x 座標
   * @param y 対象ドットの y 座標
   */
  void turnDot(bool is_on, ssize_t x, ssize_t y) {

    if (isOutOfBound(x, y))
      return;

    size_t bit_i             = BufferWidth - x - 1;
    size_t buf_i             = y;
    _buffer.at(buf_i)[bit_i] = is_on;
  }

  /**
   * @brief 指定領域をクリア
   * 
   * @param x 領域の左上隅の x 座標
   * @param y 領域の左上隅の y 座標
   * @param width 領域の幅
   * @param height 領域の高さ
   * @pre <code>width @<= std::numeric_limits<ssize_t>::max()</code>（さもなければ assert failed）
   * @pre <code>height @<= std::numeric_limits<ssize_t>::max()</code>（さもなければ assert failed）
   */
  void clear(ssize_t x, ssize_t y, size_t width, size_t height) {

    // ssize_t にキャストしても値が壊れないことを保証
    assert(width <= SSIZE_T_MAX);
    assert(height <= SSIZE_T_MAX);

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
    // x >= 0, y >= 0 が保証される

    size_t actual_width = width;
    size_t right_space  = BufferWidth_S - x - static_cast<ssize_t>(width);

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
   * @copydoc IBuffer::getHorizontialFrom(size_t, size_t, bool) const
   * @pre <code>x @<= std::numeric_limits<ssize_t>::max()</code>（さもなければ assert failed）
   * @pre <code>y @<= std::numeric_limits<ssize_t>::max()</code>（さもなければ assert failed）
   */
  virtual const uint8_t getHorizontialFrom(size_t x, size_t y, bool swap) const {

    // ssize_t にキャストしても値が壊れないことを保証
    assert(x <= SSIZE_T_MAX);
    assert(y <= SSIZE_T_MAX);

    if (isOutOfBound(x, y, 8, 1) || y >= BufferHeight_S)
      return 0;

    ssize_t            bitset_start_i = BufferWidth_S - x - 8;
    MAX7219::bitset<8> retval         = _buffer.at(y).template range<8>(bitset_start_i);

    if (swap)
      retval.swap();

    return static_cast<uint8_t>(retval.to_ulong());
  }

  /**
   * @copydoc IBuffer::getVerticalFrom(size_t, size_t, bool) const
   * @pre <code>x @<= std::numeric_limits<ssize_t>::max()</code>（さもなければ assert failed）
   * @pre <code>y @<= std::numeric_limits<ssize_t>::max()</code>（さもなければ assert failed）
   */
  virtual const uint8_t getVerticalFrom(size_t x, size_t y, bool swap) const {

    // ssize_t にキャストしても値が壊れないことを保証
    assert(x <= SSIZE_T_MAX);
    assert(y <= SSIZE_T_MAX);

    if (isOutOfBound(x, y, 1, 8))
      return 0;

    ssize_t bitset_i = BufferWidth_S - x - 1;
    if (bitset_i < 0)
      return 0;

    auto retval = MAX7219::bitset<8>();

    if (swap) {
      for (size_t i = -std::min(y, 0U); i < 8 && y + i < _buffer.size(); i++)
        retval[7 - i] = _buffer.at(y + i)[bitset_i];
    } else {
      for (size_t i = -std::min(y, 0U); i < 8 && y + i < _buffer.size(); i++)
        retval[i] = _buffer.at(y + i)[bitset_i];
    }
    return static_cast<uint8_t>(retval.to_ulong());
  }

  /**
   * @copydoc IBuffer::getBufferWidth() const
   */
  virtual const uint8_t getBufferWidth() const {
    return BufferWidth;
  }

  /**
   * @copydoc IBuffer::getBufferHeight() const
   */
  virtual const uint8_t getBufferHeight() const {
    return BufferHeight;
  }

  /**
   * @copydoc IBuffer::printToSerial()
   */
  virtual void printToSerial() const {

    // 区切り線
    Serial.println(std::string(BufferWidth, '-').c_str());

    for (size_t i = 0; i < _buffer.size(); i++) {

      auto bstr = _buffer.at(i).to_string(' ', '*');
      Serial.println(bstr.c_str());
    }
  }
};

}; // namespace MAX7219

#endif // MAX7219Display_BufferBase_H_