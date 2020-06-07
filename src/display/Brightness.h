#ifndef Brightness_H_
#define Brightness_H_

#include <Arduino.h>

// 例外が使えない環境で boost を使うので、エラーハンドラを定義して assert に置換する
// see: https://faithandbrave.hateblo.jp/entry/20101025/1287979418
#define BOOST_NO_EXCEPTIONS

#include <boost/assert.hpp>

namespace boost {
template <class E>
void throw_exception(E const &e) {
  __assert_func(PSTR(__FILE__), __LINE__, __ASSERT_FUNC, e.what());
}
} // namespace boost

#include <boost/circular_buffer.hpp>

typedef struct {
  int8_t                  manual_value;
  std::array<uint16_t, 6> thresholds;
  uint16_t                hysteresis;
} brightness_setting_t;

/**
 * @brief 光センサの観測値からLEDの明るさを算出するクラス
 * 
 */
class Brightness {

private:
  //CircularQueue<uint16_t, 8> _buffer;
  boost::circular_buffer<uint16_t> _buffer = boost::circular_buffer<uint16_t>(64);
  brightness_setting_t             _setting;
  uint16_t                         _current = 15;
  uint16_t                         _before  = 0;

  /**
   * @brief 生の観測値からLEDの明るさを計算
   * 
   * @param v 観測値
   * @return int8_t LEDの明るさ
   */
  int8_t calcBrightness(uint16_t v);

public:
  /**
   * @brief 観測値を更新する
   * 
   * @param v 新たな観測値
   * @return int8_t LEDの明るさとして設定すべき値（-1 なら off）
   */
  int8_t update(uint16_t v);
  /**
   * @brief 設定を更新する
   * 
   * @param setting 
   */
  void changeSetting(const brightness_setting_t &setting);
  int8_t getValue();
  uint16_t getRawValue();
};

#endif // Brightness_H_
