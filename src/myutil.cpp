#include "myutil.h"

#if defined(DEBUG) || defined(__PLATFORMIO_BUILD_DEBUG__)
#else
void ignoredAssert(PGM_P expression, PGM_P file, int line) {
  Serial.printf_P(PSTR("WARNING: Assertion '%s' failed, but ignored.\n at: %s:%d\n"), expression, file, line);
}
#endif


/**
 * @brief WDT reset を起こさない delayMicroseconds（多少ズレるが許容範囲）
 * 
 * @param us 待ち時間をマイクロ秒で指定
 */
void _delayMicroseconds(uint64_t us) {
  uint64_t start = micros64();
  while (micros64() - start < us)
    yield(); // avoid WDT reset
}

bool operator==(const struct tm &lhs, const struct tm &rhs) {
  return lhs.tm_year == rhs.tm_year &&
         lhs.tm_mon == rhs.tm_mon &&
         lhs.tm_mday == rhs.tm_mday &&
         lhs.tm_hour == rhs.tm_hour &&
         lhs.tm_min == rhs.tm_min &&
         lhs.tm_sec == rhs.tm_sec &&
         lhs.tm_isdst == rhs.tm_isdst;
}

bool operator!=(const struct tm &lhs, const struct tm &rhs) {
  return !(lhs == rhs);
}
