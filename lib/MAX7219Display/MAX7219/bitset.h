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
   * @brief bitset の範囲 [start, start + length) のビットをすべて 1 または 0 にする
   *        （LSB = 右端が 0 番目であることに注意）
     * 
   * @param start 範囲の始点（start >= N の場合は assert が fail）
   * @param length 範囲の長さ（start + length >= N の場合は切り捨てられる）
   * @param val true なら 1 が、false なら 0 がセットされる
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
   * @brief bitset の範囲 [start, start + length) のビットを反転させる
   *        （LSB = 右端が 0 番目であることに注意）
     * 
   * @param start 範囲の始点（start >= N の場合は assert が fail）
   * @param length 範囲の長さ（start + length >= N の場合は切り捨てられる）
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
   * @brief bitset の範囲 [start, start + Nout) を、新しい bitset<Nout> にコピーして返す
   * 
   * @tparam Nout 範囲の長さ
   * @param start 範囲の始点（start >= N の場合は assert が fail）
   * @return MAX7219::bitset<Nout> 
   * @remark start + Nout > N の場合、余った部分は 0 で埋められる
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