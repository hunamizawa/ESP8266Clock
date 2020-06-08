#ifndef ESP8266Clock_Setting_H_
#define ESP8266Clock_Setting_H_

#include <Arduino.h>
#include <MAX7219Display.h>
#include <vector>

/// @brief WiFiManager のAP名
static constexpr char AP_NAME[] = "ESP8266Clock";

/// @brief デフォルトの NTP サーバー（NICT）
static constexpr char DEFAULT_NTP_SERVER[] = "ntp.nict.jp";
static constexpr char DEFAULT_TZAREA[]     = "Asia";
static constexpr char DEFAULT_TZCITY[]     = "Tokyo";

/// @brief シリアルポートのボーレート
static constexpr unsigned long SERIAL_BAUD_RATE = 115200;

/// @brief 気温計測何回ごとに、サーバーにデータを送信するか
static constexpr uint8_t DATA_SEND_INTERVAL = 1;

/// @brief 1リクエストに最大で何回分までデータを入れるか
static constexpr uint8_t DATA_SEND_MAXCOUNT = 15;

/// @brief 気温データ送信に失敗した時など、最大で何回分のデータを溜めておくか
static constexpr size_t ENVDATA_STOCK_MAX = 64;

// SPI で使うピン番号
static constexpr int SPI_MOSI       = 13;
static constexpr int SPI_CLK        = 14;
static constexpr int SPI_CS_DISPLAY = 12; // MISO と被っても大丈夫

// I2C で使うピン番号
static constexpr int I2C_SDA = 4;
static constexpr int I2C_SCK = 2;

// SEL スイッチが繋がっているピン番号
static constexpr uint8_t PORT_SEL = 5;

/// @brief ディスプレイバッファの設定
const std::vector<MAX7219::setting_t> _buffer_settings = {
    // {X, Y, 向き, 反転}
    // ESP8266 から見て遠い順に書く
    {24, 8, MAX7219::Rotate::_0,   false},
    {16, 8, MAX7219::Rotate::_0,   false},
    {8,  8, MAX7219::Rotate::_0,   false},
    {0,  8, MAX7219::Rotate::_0,   false},
    {0,  0, MAX7219::Rotate::_180, false},
    {8,  0, MAX7219::Rotate::_180, false},
    {16, 0, MAX7219::Rotate::_180, false},
    {24, 0, MAX7219::Rotate::_180, false},
};

#endif // ESP8266Clock_Setting_H_