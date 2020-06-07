#ifndef MAX7219Display_setting_t_H_
#define MAX7219Display_setting_t_H_

#include <stdint.h>

namespace MAX7219 {

/**
 * @brief マトリックスLED の向きを表す列挙体
 * 
 */
enum class Rotate {
  _0,
  Clockwise,
  _180,
  Counterclockwise,
};

/**
 * @brief MAX7219 や マトリックスLED の接続順序や向きを定義する構造体
 * 
 */
typedef struct {
  uint8_t device_x;
  uint8_t device_y;
  Rotate  rotation;
  bool    reverse;
} setting_t;

} // namespace MAX7219

#endif // MAX7219Display_setting_t_H_