/**
 * @file TZDB.h
 */

#ifndef ESP8266Clock_TZDB_H
#define ESP8266Clock_TZDB_H

#include <Arduino.h>
#include <TZ.h>

namespace TZDB {
/**
  * @brief timezone の設定を取得、および area/city の存在判定
  * 
  * @param area 地域名
  * @param city 都市名
  * @return PGM_P (const char *) configTime() に渡す用の timezone 文字列
  * @return nullptr area/city が存在しない
  */
PGM_P getTZ(const String &area, const String &city);

} // namespace TZDB

#endif // ESP8266Clock_TZDB_H
