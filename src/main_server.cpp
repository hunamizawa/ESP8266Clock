/**
 * @file main_server.cpp
 * @brief part of the main.cpp
 */

#include "TZDB.h"
#include "const.h"
#include "main.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <map>

class MyWebServer : public ESP8266WebServer {
public:
  using ESP8266WebServer::ESP8266WebServer;

  HTTPClientStatus getStatus() const {
    return _currentStatus;
  }
};

//! HTTP サーバー
static MyWebServer _server(80);

/**
 * @brief 拡張子から MIME タイプを取得
 * 
 * @param filename ファイル名
 * @return PGM_P MIME タイプ
 */
static PGM_P getContentType_P(String filename) {
  // if (filename.endsWith(".htm")) {
  //   return "text/html";
  //} else
  if (filename.endsWith(".html")) {
    return MIME_TEXT_HTML;
  } else if (filename.endsWith(".css")) {
    return PSTR("text/css");
  } else if (filename.endsWith(".js")) {
    return PSTR("application/javascript");
    // } else if (filename.endsWith(".png")) {
    //   return PSTR("image/png");
  } else if (filename.endsWith(".gif")) {
    return PSTR("image/gif");
    // } else if (filename.endsWith(".jpg")) {
    //   return PSTR("image/jpeg");
    // } else if (filename.endsWith(".ico")) {
    //   return PSTR("image/x-icon");
    // } else if (filename.endsWith(".xml")) {
    //   return PSTR("text/xml");
    // } else if (filename.endsWith(".pdf")) {
    //   return PSTR("application/x-pdf");
    // } else if (filename.endsWith(".zip")) {
    //   return PSTR("application/x-zip");
    // } else if (filename.endsWith(".gz")) {
    //   return PSTR("application/x-gzip");
  }
  return PSTR("application/octet-stream");
}

/**
 * @brief path に対応するファイルを探し、あればレスポンスとして送る
 * 
 * @param path パス
 * @retval true ファイルがあった
 * @retval false Not Found
 */
static bool handleFileRead(String path) {
  if (path.endsWith("/"))
    path += "index.html";

  auto contentType = FPSTR(getContentType_P(path));
  if (!SPIFFS.exists(path))
    return false;

  auto file = SPIFFS.open(path, "r");
  if (!file)
    return false;

  _server.streamFile(file, contentType);
  file.close();

  return true;
}

/**
 * @brief POST で渡されたパラメータを std::map に入れる
 * 
 * @param server ESP8266WebServer のインスタンス
 * @param retval パースした結果が入る
 */
static void argsAsMap(const ESP8266WebServer &server, std::map<String, String> *retval) {
  retval->clear();
  for (int i = 0; i < server.args(); i++) {
    auto key   = server.argName(i);
    auto value = server.arg(i);
    if (!key || key.isEmpty() || key == "plain")
      continue;
    (*retval)[key] = value;
  }
}

static bool contains(const std::map<String, String> &map, const String &key) {
  return map.find(key) != map.end();
}

static void handleGetSetting() {
  String json;
  _setting.serialize(json);
  _server.send(HTTP_CODE_OK, FPSTR(MIME_APPLICATION_JSON), json);
}

static void reboot() {

  if (!handleFileRead("/wait_reboot.html")) {
    _server.send(HTTP_CODE_OK, FPSTR(MIME_TEXT_PLAIN), "Rebooting...");
  }

  while (_server.getStatus() != HC_NONE) {
    _server.handleClient();
    yield();
  }

  _server.stop();

  ESP.restart();
  delay(1000);
}

/**
 * @brief HTTP サーバーを初期化する
 * 
 */
void setupServer() {

  _server.on("/envdata", HTTP_GET, []() {
    // 方針: DynamicJsonDocument (ArduinoJson) で一気にシリアライズするとメモリ不足になるので
    // 　　  1要素ずつ DynamicJsonDocument でシリアライズし、chunk transfer で少しずつ送信する

    String              json;
    size_t              capacity = JSON_OBJECT_SIZE(5);
    DynamicJsonDocument doc(capacity);

    // Enable chunk transfer
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);

    if (_setting.ambient_writekey && _setting.ambient_writekey.length() > 0) {

      // server は、空の文字列 ("") をレスポンスの終端とみなすので、
      // _setting.ambient_writekey == "" の場合に
      // _server.sendContent(_setting.ambient_writekey) を実行すると
      // そこでリクエストのペイロードが途切れてしまう。
      // よって _setting.ambient_writekey == "" かどうかで場合分けが必要。

      _server.send_P(HTTP_CODE_OK, MIME_APPLICATION_JSON, PSTR("{\"writeKey\":\""));
      _server.sendContent(_setting.ambient_writekey);
      _server.sendContent_P(PSTR("\",\"data\":["));

    } else {

      _server.send_P(HTTP_CODE_OK, MIME_APPLICATION_JSON, PSTR("{\"writeKey\":\"\",\"data\":["));
    }

    auto need_comma = false;

    for (envdata_t data : _datas) {

      if (need_comma)
        _server.sendContent_P(PSTR(","));

      doc["created"] = data.time;
      doc["time"]    = 1;
      doc["d1"]      = ftostrf(data.temperature, 2);
      doc["d2"]      = ftostrf(data.humidity, 1);
      doc["d3"]      = ftostrf(data.pressure, 2);

      serializeJson(doc, json);
      _server.sendContent(json);

      need_comma = true;
    }

    _server.sendContent_P(PSTR("]}"));
    // End of response
    _server.sendContent("");
  });

  _server.on("/setting", HTTP_GET, handleGetSetting);

  _server.on("/setting", HTTP_POST, []() {
    auto dic = std::map<String, String>();
    argsAsMap(_server, &dic);

    for (auto &&i : dic) {
      Serial.printf("key=%s value=%s\n", i.first.c_str(), i.second.c_str());
    }

    if (contains(dic, "override_pane")) {
      auto pane              = toOverridePanes(dic.at("override_pane"));
      _setting.override_pane = pane;
      _buffer.setOverridePane(pane);
      _display.testMode(pane == OverridePanes::TEST);

    } else if (contains(dic, "pane")) {
      auto pane = toPanes(dic.at("pane"));
      if (!isValidExternalUse(pane)) {
        _server.send_P(HTTP_CODE_BAD_REQUEST, MIME_TEXT_PLAIN, PSTR("Invalid argument 'pane'"));
        return;
      }
      _setting.pane = pane;
      _buffer.setPane(pane);

    } else if (contains(dic, "tzarea") || contains(dic, "tzcity")) {
      
      if (!contains(dic, "tzarea")) {
        _server.send_P(HTTP_CODE_BAD_REQUEST, MIME_TEXT_PLAIN, PSTR("Please provide required argument 'tzarea'"));
        return;
      }
      if (!contains(dic, "tzcity")) {
        _server.send_P(HTTP_CODE_BAD_REQUEST, MIME_TEXT_PLAIN, PSTR("Please provide required argument 'tzcity'"));
        return;
      }

      auto tzarea = dic.at("tzarea");
      auto tzcity = dic.at("tzcity");

      _setting.tzarea = tzarea;
      _setting.tzcity = tzcity;

      if (!saveSetting()) {
        _server.send_P(HTTP_CODE_INTERNAL_SERVER_ERROR, MIME_TEXT_PLAIN, PSTR("An error occured while saving settings"));
        return;
      }

      reboot();
      return;

    } else if (contains(dic, "ntp1")) {

      std::vector<String> new_ntp;

      for (size_t i = 1; i <= 3; i++) {
        auto key = "ntp" + String(i);
        if (!contains(dic, key))
          continue;

        auto addr = dic.at(key);
        if (!addr || addr.isEmpty()) {
          if (i == 1) {
            // ntp1 のみ必須パラメータ
            _server.send(HTTP_CODE_BAD_REQUEST, FPSTR(MIME_TEXT_PLAIN), "Parameter 'ntp" + String(i) + "' is invalid");
            return;
          }
          continue;
        }

        // FQDN にドットが 1 つもないのはおかしい
        if (addr.indexOf('.') < 0) {
          _server.send(HTTP_CODE_BAD_REQUEST, FPSTR(MIME_TEXT_PLAIN), "Parameter 'ntp" + String(i) + "' is invalid");
          return;
        }

        new_ntp.push_back(addr);
      }

      if (new_ntp.size() == 0) {
        _server.send_P(HTTP_CODE_BAD_REQUEST, MIME_TEXT_PLAIN, PSTR("All 'ntp*' parameter are empty"));
        return;
      }

      _setting.ntp.assign(new_ntp.cbegin(), new_ntp.cend());

      if (!saveSetting()) {
        _server.send_P(HTTP_CODE_INTERNAL_SERVER_ERROR, MIME_TEXT_PLAIN, PSTR("An error occured while saving settings"));
        return;
      }

      reboot();
      return;

    } else {
      _server.send_P(HTTP_CODE_BAD_REQUEST, MIME_TEXT_PLAIN, PSTR("Unknown or empty params"));
      return;
    }

    // 変更後の設定をJSONで返す
    handleGetSetting();
  });

  // /adc ―― 最新の ADC の値を返す
  _server.on("/adc", []() {
    auto v = _bn.calcAverageRawValue();
    _server.send(HTTP_CODE_OK, MIME_TEXT_PLAIN, String(v));
  });

  // パスに対するハンドラが定義されていない場合、SPIFFS にあるファイルを返そうとしてみる
  _server.onNotFound([]() {
    if (handleFileRead(_server.uri()))
      return;
    _server.send_P(HTTP_CODE_NOT_FOUND, MIME_TEXT_PLAIN, PSTR("File Not Found"));
  });

  // Start listen
  _server.begin();
}

void yieldServer() {
  _server.handleClient();
}