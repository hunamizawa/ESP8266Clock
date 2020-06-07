#ifndef MAX7219Display_Buffer_H_
#define MAX7219Display_Buffer_H_

#include "BufferBase.h"
#include "EmptyGraphics.h"
#include <string>
using std::u16string;

namespace MAX7219 {

/**
 * @brief MAX7219::BufferBase を継承し、文字・数値の書き込みといった高度な機能を提供するクラス
 * 
 * @tparam BufferWidth バッファ領域の幅
 * @tparam BufferHeight バッファ領域の高さ
 * @tparam TGraphics 文字グリフ（フォント）を Buffer に提供してくれるクラス。ユーザーが定義することを想定している
 */
template <size_t BufferWidth, size_t BufferHeight, class TGraphics = EmptyGraphics>
class Buffer : public BufferBase<BufferWidth, BufferHeight> {

private:
  TGraphics graphics;

  /**
   * @brief 指定範囲に描画可能な文字数を計算する
   * 
   * @param width 領域の幅
   * @param cwidth 文字幅
   * @return size_t 描画可能な文字数
   */
  size_t calcAvailableChars(size_t width, size_t cwidth) const {
    for (size_t i = 1;; i++) {
      if (i * (cwidth + 1) - 1 > width)
        return i - 1;
    }
  }

  /**
   * @brief 指定の文字列を描画するのに必要な領域の幅を計算する
   * 
   * @param width 文字列
   * @param cwidth 文字幅
   * @return size_t 必要な描画領域の幅
   */
  template <uint8_t CWidth, uint8_t CHeight, class TString>
  size_t calcRequiredWidth(TString str) {
    
    if (graphics.template tryGetGlyph<CWidth, CHeight>(u'.', nullptr))
      return str.length() * (CWidth + 1) - 1;

    size_t retval = 0;
    for (size_t i = 0; i < str.length(); i++){
      auto c = str[i];
      if (c == u'.')
        retval += 2;
      else
        retval += CWidth + 1;
    }

    return retval - 1;
  }

public:
  // Disallow copy
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  // Allow default move
  Buffer(Buffer &&) = default;
  Buffer &operator=(Buffer &&) = default;

  Buffer() {}
  virtual ~Buffer() = default;

  /**
   * @brief 指定位置に文字を描画
   * 
   * @tparam CWidth 文字の幅
   * @tparam CHeight 文字の高さ
   * @param c 描画する文字
   * @param x 描画領域左上の x 座標
   * @param y 描画領域左上の y 座標
   */
  template <uint8_t CWidth, uint8_t CHeight>
  size_t writeChar(const char16_t c, ssize_t x, ssize_t y) {

    std::array<uint8_t, CHeight> buf;

    if (graphics.template tryGetGlyph<CWidth, CHeight>(c, buf.data())) {
      this->write(buf.data(), x, y, CWidth, CHeight);
      return CWidth;

    } else if (c == u'.') {
      this->turnDot(true, x, y + CHeight - 1);
      return 1;
    }

    // グリフが無い文字は空白にする
    return CWidth;
  }

  /**
   * @brief 指定位置に文字列を描画
   * 
   * @tparam CWidth 文字の幅
   * @tparam CHeight 文字の高さ
   * @param str 描画する文字列
   * @param x 描画領域左上の x 座標
   * @param y 描画領域左上の y 座標
   * @param charSpace 字間
   */
  template <uint8_t CWidth, uint8_t CHeight>
  void writeString(const String &str, ssize_t x, ssize_t y, ssize_t charSpace = 1) {
    ssize_t char_x = x;
    for (size_t i = 0; i < str.length(); i++)
      char_x += writeChar<CWidth, CHeight>(str[i], char_x, y) + charSpace;
  }

  /**
   * @brief 指定位置に文字列を描画
   * 
   * @tparam CWidth 文字の幅
   * @tparam CHeight 文字の高さ
   * @param str 描画する文字列
   * @param x 描画領域左上の x 座標
   * @param y 描画領域左上の y 座標
   * @param charSpace 字間
   */
  template <uint8_t CWidth, uint8_t CHeight>
  void writeString(const std::u16string &str, ssize_t x, ssize_t y, ssize_t charSpace = 1) {
    ssize_t char_x = x;
    for (size_t i = 0; i < str.length(); i++)
      char_x += writeChar<CWidth, CHeight>(str.at(i), char_x, y) + charSpace;
  }

  /**
   * @brief 指定位置に整数を右詰めで描画
   * 
   * @tparam CWidth 文字の幅
   * @tparam CHeight 文字の高さ
   * @param value 描画する数値
   * @param zero_pad 1 以上を指定すると、その桁数になるように 0 埋めする
   * @param x 描画領域左上の x 座標
   * @param y 描画領域左上の y 座標
   * @param width 描画領域の幅
    * @param charSpace 字間
  */
  template <uint8_t CWidth, uint8_t CHeight>
  void writeInteger(long value, uint8_t zero_pad, ssize_t x, ssize_t y, size_t width, ssize_t charSpace = 1) {

    static constexpr size_t buf_size = 2 + 8 * sizeof(long);

    String str;
    if (zero_pad) {
      char buf[buf_size];
      sprintf(buf, "%0*ld", std::min(buf_size - 3, static_cast<size_t>(zero_pad)), value);
      str = buf;
    } else {
      str = String(value);
    }

    // 文字列が width に収まるように切り詰める
    size_t real_width = width;
    while (str.length() > 0 && (real_width = calcRequiredWidth<CWidth, CHeight>(str)) > width)
      str = str.substring(1, str.length());

    writeString<CWidth, CHeight>(str, x + width - real_width, y, charSpace);
  }

  /**
   * @brief 指定位置に実数を右詰めで描画
   * 
   * @tparam CWidth 文字の幅
   * @tparam CHeight 文字の高さ
   * @param value 描画する数値
   * @param prec_max 小数部の最大桁数
   * @param x 描画領域左上の x 座標
   * @param y 描画領域左上の y 座標
   * @param width 描画領域の幅
   * @param charSpace 字間
   */
  template <uint8_t CWidth, uint8_t CHeight>
  void writeReal(double value, uint8_t prec_max, ssize_t x, ssize_t y, size_t width, ssize_t charSpace = 1) {

    int    prec       = std::min(static_cast<int>(prec_max), 30);
    size_t real_width = width;
    String str;

    // 文字列が width に収まるよう、まず小数点以下を切り詰める
    for (; prec >= 0; prec--) {
      str = String(value, prec);
      if ((real_width = calcRequiredWidth<CWidth, CHeight>(str)) <= width)
        break;
    }

    if (prec < 0) {
      // prec == 0 でもはみ出す場合は、上位桁を切り落とす
      while (str.length() > 0 && (real_width = calcRequiredWidth<CWidth, CHeight>(str)) > width)
        str = str.substring(1, str.length());
    }

    writeString<CWidth, CHeight>(str, x + width - real_width, y, charSpace);
  }
};

} // namespace MAX7219

#endif // MAX7219Display_Buffer_H_