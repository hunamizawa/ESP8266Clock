/**
 * @file myutil.h
 * @brief 便利関数置き場
 */

#ifndef ESP8266Util_H_
#define ESP8266Util_H_

#include <Arduino.h>
#include <assert.h>

/**
 * @brief 二重開放防止のため、クラスのコピーを禁止する
 * see: https://cpprefjp.github.io/lang/cpp11/defaulted_and_deleted_functions.html
 *      http://yohshiy.blog.fc2.com/blog-entry-335.html
 */
#define DISALLOW_COPY(...)                   \
  __VA_ARGS__(const __VA_ARGS__ &) = delete; \
  __VA_ARGS__ &operator=(const __VA_ARGS__ &) = delete;

/**
 * @brief ムーブコンストラクタをデフォルト定義する
 * see: https://cpprefjp.github.io/lang/cpp11/defaulted_and_deleted_functions.html
 *      http://yohshiy.blog.fc2.com/blog-entry-335.html
 */
#define ALLOW_DEFAULT_MOVE(...)          \
  __VA_ARGS__(__VA_ARGS__ &&) = default; \
  __VA_ARGS__ &operator=(__VA_ARGS__ &&) = default;

#if defined(DEBUG) || defined(__PLATFORMIO_BUILD_DEBUG__)
// Debug build では panic、Release build では警告を Serial に出して続行
#define assert_debug(__e) assert(__e)
#else
// Debug build では panic、Release build では警告を Serial に出して続行
#define assert_debug(__e) ((__e) ? (void)0 \
                              : ignoredAssert(PSTR(#__e), PSTR(__FILE__), __LINE__))

void ignoredAssert(PGM_P expression, PGM_P file, int line);
#endif

/**
 * @brief float を四捨五入して int にキャストする
 * 
 * @param x 
 * @return int 
 */
static inline int round(float x) {
  return static_cast<int>(std::round(x));
}

/**
 * @brief 小数の桁数を指定して float を文字列に変換
 * 
 * @param val 数値
 * @param prec 小数の桁数
 * @return String 結果
 */
String ftostrf(float val, unsigned char prec);

/**
 * @brief WDT reset を起こさない delayMicroseconds（多少ズレるが許容範囲）
 * 
 * @param us 待ち時間をマイクロ秒で指定
 */
void _delayMicroseconds(uint64_t us);

bool operator==(const struct tm &lhs, const struct tm &rhs);

bool operator!=(const struct tm &lhs, const struct tm &rhs);

#endif // ESP8266Util_H_