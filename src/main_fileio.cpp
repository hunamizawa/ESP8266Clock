/**
 * @file main_fileio.cpp
 * @brief part of the main.cpp
 */

#include "main.h"
#include <FS.h>

/**
 * @brief 設定ファイルから設定を読み込む
 * 
 */
void readSetting() {
  const size_t        capacity = JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(8) + 1090;
  DynamicJsonDocument doc(capacity);

  File f = SPIFFS.open("/setting", "r");
  if (f) {
    auto json = f.readString();
    _setting.deserialize(json);
  } else {
    // 読み込みに失敗したらデフォルト値を使う
    _setting.resetToDefault();
  }
}

/**
 * @brief 現在の設定をファイルに保存する
 * 
 * @retval true 成功
 * @retval false 不成功
 */
bool saveSetting() {
  File f = SPIFFS.open("/setting", "w");
  if (!f)
    return false;
  _setting.serialize(f);
  f.close();
  return true;
}
