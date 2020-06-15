/**
 * @file Display.h
 */

#ifndef MAX7219Display_Display_H_
#define MAX7219Display_Display_H_

#include "IBuffer.h"
#include "setting_t.h"
#include <stdint.h>

namespace MAX7219 {

/**
 * @brief IBuffer が保持しているグラフィックを実際に MAX7219 に送信するクラス
 * 
 */
class Display {
private:
  const int                    _pin_cs;
  const IBuffer *              _buffer;  //! グラフィックを保持している IBuffer オブジェクト
  const std::vector<setting_t> _devices; //! MAX7219 モジュールの相対位置や順序の設定

  /**
   * @brief すべての MAX7219 に同じ命令を送る
   * 
   * @param address レジスタアドレス
   * @param data データ
   */
  void broadcast(uint8_t address, uint8_t data) const;

public:
  /**
   * @brief Construct a new Display object
   * 
   * @param buffer グラフィックを保持している Buffer オブジェクト
   * @param pin_cs SPI CS ピン番号
   * @param devices デイジーチェーン接続された MAX7219 モジュールの相対位置や順序の設定
   * @note @c devices の順序は、マスター (ESP8266) から見て遠い順に書く。
   */
  Display(const int pin_cs, const IBuffer &buffer, const std::vector<setting_t> &devices);
  /**
   * @brief MAX7219 を初期化する
   */
  void init() const;
  /**
   * @brief TEST MODE（全ドット点灯）の入切
   * 
   * @param value true テストモード
   * @param value false 通常モード
   */
  void testMode(bool value) const;
  /**
   * @brief SHUTDOWN MODE（全ドット消灯）の入切
   * 
   * @param value true シャットダウン
   * @param value false 復帰
   */
  void shutdownMode(bool value) const;
  /**
   * @brief 明るさを設定
   * 
   * @param intensity 明るさ（0 から 15 まで）
   * @note 全てのデバイスに同じ値が設定される。
   */
  void setIntensity(uint8_t intensity) const;
  /**
   * @brief 明るさを設定
   * 
   * @param intensities デバイスごとの明るさ（0 から 15 まで）
   * @pre @c intensities の要素数は、コンストラクタ Display::Display() の引数 @c devices の要素数と一致しなければならない。
   */
  void setIntensity(std::vector<uint8_t> intensities) const;
  /**
   * @brief 全ドットクリア（IBuffer の内容は変化しない）
   */
  void clearAll() const;
  /**
   * @brief IBuffer の現在の内容を MAX7219 に送る
   */
  void send() const;
};

}; // namespace MAX7219

#endif // MAX7219Display_Display_H_