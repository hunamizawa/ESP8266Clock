/**
 * @file setting_t.h
 */

#ifndef MAX7219Display_setting_t_H_
#define MAX7219Display_setting_t_H_

#include <stdint.h>

namespace MAX7219 {

/**
 * @brief マトリックスLED の向きを表す列挙体
 */
enum class Rotate {
  _0,
  Clockwise,
  _180,
  Counterclockwise,
};

/**
 * @brief MAX7219 や マトリックスLED の接続順序や向きを定義する構造体
 */
typedef struct {
  //! バッファ上における、マトリックスLED の左上 x 座標
  size_t topleft_x;
  //! バッファ上における、マトリックスLED の左上 y 座標
  size_t topleft_y;
  //! マッピング時に回転させるかどうか
  Rotate rotation;
  //! 反転させるかどうか（ Rotate::_0 または Rotate::_180 では左右反転、それ以外では上下反転）
  bool reverse;
} setting_t;

} // namespace MAX7219

#endif // MAX7219Display_setting_t_H_