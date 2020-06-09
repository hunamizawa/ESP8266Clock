/**
 * @file EmptyGraphics.h
 */

#ifndef MAX7219Display_EmptyGraphics_H_
#define MAX7219Display_EmptyGraphics_H_

#include <Arduino.h>

namespace MAX7219 {

/**
 * @brief 文字グリフを持たない、空の Graphics
 */
class EmptyGraphics {
  /**
   * @brief 文字 @c c に対応するグリフを、uint8_t 配列として取得
   * 
   * @tparam Width 文字の幅
   * @tparam Height 文字の高さ
   * @param c 文字
   * @param retval 戻り値。長さ @e Height の uint8_t 配列が渡される
   * @return true 文字 @c c に対応するグリフが存在する。 @c retval には有効な値が含まれる。
   * @return false 文字 @c c に対応するグリフが存在しない。 @c retval の値は未定義。
   */
public:
  template <uint8_t Width, uint8_t Height>
  bool tryGetGlyph(char16_t c, uint8_t *retval) const {
    return false;
  }
};

} // namespace MAX7219

#endif // MAX7219Display_EmptyGraphics_H_