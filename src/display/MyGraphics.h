/**
 * @file MyGraphics.h
 */

#ifndef MyGraphics_H_
#define MyGraphics_H_

#include <Arduino.h>
#include <array>

class MyGraphics {
public:
  /**
 * @brief 指定サイズの文字グリフを取得
 * 
 * @tparam Width 幅（ドット）
 * @tparam Height 高さ（ドット）= uint8_t[] 配列の要素数
 * @param c 取り出したい文字
 * @param[out] retval 受け渡し用配列の先頭アドレス
 * @retval true 文字が見つかった
 * @retval false 文字が見つからない
 */
  template <uint8_t Width, uint8_t Height>
  bool tryGetGlyph(char16_t c, uint8_t *retval) const;
};

namespace ConstGraphics {

//! 「ESP8266Clock」
extern const std::array<uint32_t, 16> welcome PROGMEM;

//! 「同期中」
extern const std::array<uint32_t, 8> douki_chu PROGMEM;

//! 「設定して下さい」
extern const std::array<uint32_t, 16> plz_setting PROGMEM;

//! 「Wi-Fi 接続不可」
extern const std::array<uint32_t, 16> con_fail PROGMEM;

} // namespace ConstGraphics

#endif // MyGraphics_H_