/**
 * @file bitset.h
 */

#ifndef MAX7219Display_bitset_H_
#define MAX7219Display_bitset_H_

#include <Arduino.h>
#include <assert.h>
#include <bitset>

namespace MAX7219 {

/**
 * @brief std::bitset<N> の拡張
 * 
 * @tparam N 要素数
 */
template <size_t N>
class bitset : public std::bitset<N> {
private:
  using TBase = std::bitset<N>;

  TBase createRangeMask(size_t start, size_t length) {
    auto mask_str = std::string(length, '1') + std::string(start, '0');
    return TBase(mask_str);
  }

public:
  using std::bitset<N>::bitset;

  /**
   * @brief bitset の範囲 <code>[start, start + length)</code> のビットをすべて 1 または 0 にする
   * 
   * @param start 範囲の始点
   * @param length 範囲の長さ
   * @param val true なら 1 が、false なら 0 が指定の範囲にセットされる
   * @pre <code>start @< N</code>（さもなければ assert failed）
   * @note LSB = 右端が 0 番目であることに注意
   */
  void setRange(size_t start, size_t length, bool val = true) {
    assert(start < N);
    auto mask = createRangeMask(start, length);
    if (val)
      *this |= mask;
    else
      *this &= ~mask;
  }

  /**
   * @brief bitset の範囲 <code>[start, start + length)</code> のビットを反転させる
   * 
   * @param start 範囲の始点
   * @param length 範囲の長さ
   * @pre <code>start @< N</code>（さもなければ assert failed）
   * @note LSB = 右端が 0 番目であることに注意
   */
  void flipRange(size_t start, size_t length) {
    assert(start < N);
    auto mask = createRangeMask(start, length);
    *this ^= mask;
  }

  /**
   * @brief MSB と LSB を入れ替える
   */
  void swap() {
    auto old = MAX7219::bitset<N>(*this);
    for (size_t i = 0; i < N; i++)
      (*this)[i] = old[N - i - 1];
  }

  /**
   * @brief bitset の範囲 <code>[start, start + Nout)</code> を、要素数 @e Nout の新しい bitset にコピーして返す
   * 
   * @tparam Nout 範囲の長さ
   * @param start 範囲の始点
   * @retval MAX7219::bitset<Nout> 
   * @pre <code>start @< N</code>（さもなければ assert failed）
   * @pre <code>start + Nout @<= N</code>（さもなければ、余った部分は 0 で埋められる）
   * @note LSB = 右端が 0 番目であることに注意
   */
  template <size_t Nout>
  MAX7219::bitset<Nout> range(size_t start) const {
    assert(start < N);
    auto retval = MAX7219::bitset<Nout>();
    for (size_t i = 0; i < Nout && start + i < N; i++)
      retval[i] = (*this)[start + i];
    return retval;
  }
};

} // namespace MAX7219

#endif // MAX7219Display_bitset_H_