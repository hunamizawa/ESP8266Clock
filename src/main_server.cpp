/**
 * @file main_server.cpp
 * @brief part of the main.cpp
 */

#include "TZDB.h"
#include "const.h"
#include "main.h"
#include "resource.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <array>
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
//! OTA Update
static ESP8266HTTPUpdateServer _updater;

static constexpr char IF_NONE_MATCH[] = "If-None-Match";

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
static bool handleFileRead(String path, HTTPMethod method, const String &if_none_match) {

  if (method != HTTP_GET && method != HTTP_HEAD) {
    _server.send_P(HTTP_CODE_METHOD_NOT_ALLOWED, MIME_TEXT_PLAIN, PSTR("Method Not Allowed"));
    return true;
  }

  if (path.endsWith("/"))
    path += "index.html";

  auto contentType = getContentType_P(path);

  auto resource = Resource::searchByPath(path);
  if (resource.pointer) {

    // If-None-Match ヘッダが一致する場合は 304 Not Modified を返す
    auto etag = FPSTR(resource.etag);
    if (if_none_match == etag) {
      _server.send(HTTP_CODE_NOT_MODIFIED, contentType, "");
      return true;
    }

    _server.sendHeader("ETag", etag);
    if (method == HTTP_GET) {
      _server.send_P(HTTP_CODE_OK, contentType, resource.pointer, resource.length);
    } else { // HTTP_HEAD
      _server.setContentLength(resource.length);
      _server.send(HTTP_CODE_OK, contentType, "");
    }

    return true;
  }

  if (!SPIFFS.exists(path))
    return false;

  auto file = SPIFFS.open(path, "r");
  if (!file)
    return false;

  _server.streamFile(file, FPSTR(contentType), method);
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

static void reboot() {

  if (!handleFileRead("/wait_reboot.html", HTTP_GET, "")) {
    _server.send_P(HTTP_CODE_OK, MIME_TEXT_PLAIN, PSTR("Rebooting..."));
  }

  while (_server.getStatus() != HC_NONE) {
    _server.handleClient();
    yield();
  }

  _server.stop();

  ESP.restart();
  delay(1000);
}

static void handleGetEnvdata() {

  auto method = _server.method();
  if (method != HTTP_GET && method != HTTP_HEAD) {
    _server.send_P(HTTP_CODE_METHOD_NOT_ALLOWED, MIME_TEXT_PLAIN, PSTR("Method Not Allowed"));
    return;
  }

  // 方針: DynamicJsonDocument (ArduinoJson) で一気にシリアライズするとメモリ不足になるので
  // 　　  1要素ずつ DynamicJsonDocument でシリアライズし、chunk transfer で少しずつ送信する

  // Enable chunk transfer
  _server.setContentLength(CONTENT_LENGTH_UNKNOWN);

  if (method == HTTP_HEAD) {
    _server.send(HTTP_CODE_OK, MIME_TEXT_PLAIN, "");
    return;
  }

  String                  json;
  static constexpr size_t capacity = JSON_OBJECT_SIZE(5);
  DynamicJsonDocument     doc(capacity);

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

    // serializeJson は Append 動作なので、変数 json を for　の外に出してはいけない
    String json;

    if (need_comma)
      _server.sendContent_P(PSTR(","));

    doc["created"] = data.time;
    doc["time"]    = 1;
    doc["d1"]      = ftostrf(data.temperature, 2);
    doc["d2"]      = ftostrf(data.humidity, 1);
    doc["d3"]      = ftostrf(data.pressure, 2);

    serializeJson(doc, json);
    _server.sendContent(json);
    doc.clear();

    need_comma = true;
  }

  _server.sendContent_P(PSTR("]}"));
  // End of response
  _server.sendContent("");
}

static void handleGetSetting() {
  String json;
  _setting.serialize(json);
  _server.send(HTTP_CODE_OK, FPSTR(MIME_APPLICATION_JSON), json);
}

static inline void badRequest_P(PGM_P message) {
  _server.send_P(HTTP_CODE_BAD_REQUEST, MIME_TEXT_PLAIN, message);
}

static void handlePostSetting() {

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
      badRequest_P(PSTR("Invalid argument 'pane'"));
      return;
    }
    _setting.pane = pane;
    _buffer.setPane(pane);

  } else if (contains(dic, "tzarea") || contains(dic, "tzcity")) {

    if (!contains(dic, "tzarea")) {
      badRequest_P(PSTR("Please provide required argument 'tzarea'"));
      return;
    }
    if (!contains(dic, "tzcity")) {
      badRequest_P(PSTR("Please provide required argument 'tzcity'"));
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

      // 引数の簡易チェック
      // FQDN にドットが 1 つもないのはおかしい
      if (addr.indexOf('.') < 0) {
        _server.send(HTTP_CODE_BAD_REQUEST, FPSTR(MIME_TEXT_PLAIN), "Parameter 'ntp" + String(i) + "' is invalid");
        return;
      }

      new_ntp.push_back(addr);
    }

    if (new_ntp.size() == 0) {
      badRequest_P(PSTR("All 'ntp*' parameter are empty"));
      return;
    }

    _setting.ntp.assign(new_ntp.cbegin(), new_ntp.cend());

    if (!saveSetting()) {
      _server.send_P(HTTP_CODE_INTERNAL_SERVER_ERROR, MIME_TEXT_PLAIN, PSTR("An error occured while saving settings"));
      return;
    }

    reboot();
    return;

  } else if (contains(dic, "elev")) {

    auto elev = static_cast<float>(atof(dic.at("elev").c_str()));
    _setting.elev = elev;

    if (!saveSetting()) {
      _server.send_P(HTTP_CODE_INTERNAL_SERVER_ERROR, MIME_TEXT_PLAIN, PSTR("An error occured while saving settings"));
    }

  } else {
    badRequest_P(PSTR("Unknown or empty params"));
    return;
  }

  // 変更後の設定をJSONで返す
  handleGetSetting();
}

/**
 * @brief HTTP サーバーを初期化する
 */
void setupServer() {

  _updater.setup(&_server);

  _server.on("/envdata", handleGetEnvdata);

  _server.on("/setting", HTTP_GET, handleGetSetting);

  _server.on("/setting", HTTP_POST, handlePostSetting);

  // /adc ―― 最新の ADC の値を返す
  _server.on("/adc", []() {
    auto v = _bn.calcAverageRawValue();
    _server.send(HTTP_CODE_OK, MIME_TEXT_PLAIN, String(v));
  });

  // パスに対するハンドラが定義されていない場合、SPIFFS にあるファイルを返そうとしてみる
  _server.onNotFound([]() {
    if (handleFileRead(_server.uri(), _server.method(), _server.header(IF_NONE_MATCH)))
      return;

    _server.send_P(HTTP_CODE_NOT_FOUND, MIME_TEXT_PLAIN, PSTR("File Not Found"));
  });

  // If-None-Match ヘッダを取得する
  std::array<const char *, 1> collect_headers = {IF_NONE_MATCH};
  _server.collectHeaders(collect_headers.data(), collect_headers.size());

  // Start listen
  _server.begin();
}

void yieldServer() {
  _server.handleClient();
}