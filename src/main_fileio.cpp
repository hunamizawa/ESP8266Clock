/**
 * @file main_fileio.cpp
 * @brief part of the main.cpp
 */

#include "main.h"
#include <FS.h>

/**
 * @brief 設定ファイルから設定を読み込む
 */
void readSetting() {

  File f = SPIFFS.open(PATH_OF_SETTING, "r");
  if (f) {
    auto json = f.readString();
    _setting.deserialize(json);
  } else {
    // 読み込みに失敗したらデフォルト値を使う
    Serial.println("WARNING: Failed to read file '/setting'.");
    _setting.resetToDefault();
  }
  
  _bn.changeSetting(_setting.brightness);
}

/**
 * @brief 現在の設定をファイルに保存する
 * 
 * @retval true 成功
 * @retval false 不成功
 */
bool saveSetting() {
  
  File f = SPIFFS.open(PATH_OF_SETTING, "w");
  if (!f)
    return false;
  _setting.serialize(f);
  f.close();
  return true;
}
