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
   */
  template <uint8_t CWidth, uint8_t CHeight>
  void writeString(const String &str, ssize_t x, ssize_t y) {
    ssize_t char_x = x;
    for (size_t i = 0; i < str.length(); i++)
      char_x += writeChar<CWidth, CHeight>(str[i], char_x, y) + 1;
  }

  /**
   * @brief 指定位置に文字列を描画
   * 
   * @tparam CWidth 文字の幅
   * @tparam CHeight 文字の高さ
   * @param str 描画する文字列
   * @param x 描画領域左上の x 座標
   * @param y 描画領域左上の y 座標
   */
  template <uint8_t CWidth, uint8_t CHeight>
  void writeString(const std::u16string &str, ssize_t x, ssize_t y) {
    ssize_t char_x = x;
    for (size_t i = 0; i < str.length(); i++)
      char_x += writeChar<CWidth, CHeight>(str.at(i), char_x, y) + 1;
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
   */
  template <uint8_t CWidth, uint8_t CHeight>
  void writeInteger(long value, uint8_t zero_pad, ssize_t x, ssize_t y, size_t width) {

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

    writeString<CWidth, CHeight>(str, x + width - real_width, y);

    // auto is_minus         = value < 0;
    // auto int_part_s       = String(value);
    // auto avail_digs_count = calcAvailableChars(width, CWidth);
    //
    // // 描画領域右端の x 座標
    // ssize_t right_edge_x = x + width;
    //
    // // 下位桁から描画
    // for (size_t i = 0; i < avail_digs_count; i++) {
    //
    //   // この桁の左上 x 座標
    //   ssize_t dig_x = right_edge_x + 1 - (i + 1) * (CWidth + 1);
    //
    //   if (i < int_part_s.length()) {
    //     writeChar<CWidth, CHeight>(int_part_s[int_part_s.length() - (i + 1)], dig_x, y);
    //     continue;
    //   }
    //
    //   if (zero_pad && !is_minus) {
    //     writeChar<CWidth, CHeight>(u'0', dig_x, y);
    //     continue;
    //   }
    //
    //   break;
    // }
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
   */
  template <uint8_t CWidth, uint8_t CHeight>
  void writeReal(double value, uint8_t prec_max, ssize_t x, ssize_t y, size_t width) {

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

    writeString<CWidth, CHeight>(str, x + width - real_width, y);

    // auto is_minus = value < 0.0f;
    //
    // float int_part_f;
    // auto  frac_part = std::modf(std::abs(value), &int_part_f);
    // auto  int_part  = static_cast<long>(int_part_f);
    // if (is_minus)
    //   int_part = -int_part;
    //
    // auto avail_digs_count_i = calcAvailableChars(width, CWidth);
    // auto int_part_s         = String(int_part);
    //
    // // 描画できる小数点以下の桁数を計算
    // auto remain_space = static_cast<ssize_t>(width) - static_cast<ssize_t>(calcRequiredWidth(int_part_s, CWidth));
    // auto prec         = calcAvailableChars(remain_space, CWidth);

    // if (prec != 0 && (int_part_s.length() + prec) * (CWidth + 1) + 1 > width) {
    //   --prec;
    // }
    //
    // if (prec == 0) {
    //   writeDecimal<CWidth, CHeight>(value_rounded, zero_pad, x, y, width);
    //   return;
    // }
    //
    // // 使用領域左端をゼロとしたときの、小数点の x 座標
    // size_t dot_dx = x + width - prec * (CWidth + 1) - 1;
    //
    // this->clear(x, y, width, CHeight);
    //
    // writeDecimal<CWidth, CHeight>(static_cast<long>(int_part), zero_pad, x, y, dot_dx - 1);
    //
    // auto frac_part_s = String(std::abs(frac_part));
    //
    // for (uint8_t i = 0; i < prec; i++) {
    //
    //   size_t dig_dx = x + dot_dx + 2 + i * (CWidth + 1);
    //
    //   if (frac_part_s.length() >= i + 2)
    //     writeChar<CWidth, CHeight>(u'0', dig_dx, y);
    //   else
    //     writeChar<CWidth, CHeight>(frac_part_s[i + 2], dig_dx, y);
    // }
  }
};

} // namespace MAX7219

#endif // MAX7219Display_Buffer_H_