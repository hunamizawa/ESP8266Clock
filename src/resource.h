/**
 * @file resource.h
 */

#ifndef ESP8266Clock_resource_H_
#define ESP8266Clock_resource_H_

#include "resource-data.h"
#include <Arduino.h>

namespace Resource {

/**
 * @brief 埋め込みリソースの情報を表す構造体
 */
typedef struct {
  //! PROGMEM に埋め込まれた char 型配列の先頭アドレス
  PGM_P        pointer;
  //! 配列 @c pointer の要素数
  const size_t length;
  //! HTTP でレスポンスする時に使う ETag。中身は文字列（SHA-256）
  PGM_P        etag;
} resource_t;

/**
 * @brief ファイルパスから埋め込みリソースを検索
 * 
 * @param path パス
 * @return 埋め込みリソースの情報。 resource_t::pointer == nullptr の場合はリソースが存在しない。
 */
resource_t searchByPath(const String &path);

} // namespace Resource

#endif // ESP8266Clock_resource_H_
