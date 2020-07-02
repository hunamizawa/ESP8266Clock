/**
 * @file main.h
 */

#ifndef ESP8266Clock_main_H_
#define ESP8266Clock_main_H_

#include "ClockSetting.h"
#include "const.h"
#include "display/Brightness.h"
#include "display/MyBuffer.h"
#include "myutil.h"
#include "setting.h"
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESPPerfectTime.h>
#include <MAX7219Display.h>

static constexpr char   PATH_OF_SETTING[]           = "/setting";
static constexpr size_t ADD_BYTES_SERIALIZE_ENVDATA = 43;

//! ユーザーが変更可能な時計の動作設定
extern ClockSetting _setting;

// main_display

extern MyBuffer         _buffer;
extern MAX7219::Display _display;
extern Brightness       _bn;

void updateDisplay(const struct tm &tm, suseconds_t usec);
void readAndSetBrightness();
void changePaneIfSELPushed();
void displayAndBufferInit();
void startWelcomeDisplay();
void stopWelcomeDisplay();

// main_fileio

void readSetting();
bool saveSetting();

// main_server

void setupServer();
void yieldServer();

// main_network

void   connectWiFi();
size_t postEnvdatas(const String &addr, const String &writeKey, const size_t start, const size_t maxLength);

// main_bme

using EnvDataQueue = boost::circular_buffer<envdata_t>;

extern EnvDataQueue _datas;
extern envdata_t    _last_envdata;

bool bmeInit();
void startMeasureEnvironment(const struct tm &tm);
void readEnvironment();

#endif // ESP8266Clock_main_H_