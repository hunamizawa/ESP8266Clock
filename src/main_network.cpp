/**
 * @file main_network.cpp
 * @brief part of the main.cpp
 */

#include "main.h"
#include <WiFiManager.h>

/**
 * @brief WiFiManager が設定モードに入った時に呼ばれるコールバック関数
 * 
 * @param wiFiManager 
 */
static void wifiConfigModeCallback(WiFiManager *wiFiManager) {

  // display にメッセージを表示
  stopWelcomeDisplay();
  _buffer.setPane(Panes::REQUIRE_SETTING);
  _buffer.update({0}, 0, {0}, nullptr);
  _display.send();

  // 確認用にシリアルに SSID と URL を流しておく
  auto ssid   = wiFiManager->getConfigPortalSSID();
  auto ipaddr = WiFi.softAPIP().toString();

  Serial.println("Require Wi-Fi config.");
  Serial.println("Connect Wi-Fi access point '" + ssid + "',");
  Serial.println("then access to http://" + ipaddr + "/ in your browser.");
}

/**
 * @brief Wi-Fi がタイムアウトしたので ESP をリセットする
 * 
 */
static void failedToConnect() {

  Serial.println("failed to connect and hit timeout");
  stopWelcomeDisplay();
  _buffer.setPane(Panes::CONNECT_FAILED);
  _buffer.update({0}, 0, {0}, nullptr);
  _display.send();
  delay(3000);

  // reset and try again
  ESP.restart();
  delay(1000);
}

/**
 * @brief WiFiManager を利用して Wi-Fi 接続する
 * 必要に応じて自動的に設定モード（SoftAP モード）に入る
 */
void connectWiFi() {
  WiFiManager wifiManager;
  wifiManager.setAPCallback(wifiConfigModeCallback);

  if (digitalRead(PORT_SEL) == 0) {
    // SEL ボタンが押されている場合、強制的に設定モードに入る
    if (!wifiManager.startConfigPortal(AP_NAME)) {
      failedToConnect();
    }
  } else if (!wifiManager.autoConnect(AP_NAME)) {
    failedToConnect();
  }
}

static int httpPost(String address, const String &contentType, const String &payload) {

  static HTTPClient http;
  static WiFiClient client;

  http.begin(client, address);
  http.addHeader("Content-Type", contentType);
  return http.POST(payload);
}

/**
 * @brief values を Ambient に送信する形式で retval にシリアライズ
 * 
 * @param values シリアライズするデータ（10個以下推奨）
 * @param[out] retval JSON 文字列
 */
static void serializeEnvDatas(const size_t count, const String &writeKey, String *retval) {

  size_t              capacity = JSON_ARRAY_SIZE(count) + JSON_OBJECT_SIZE(2) + count * JSON_OBJECT_SIZE(5) + ADD_BYTES_SERIALIZE_ENVDATA * count + 10 + writeKey.length();
  DynamicJsonDocument doc(capacity);

  doc["writeKey"] = writeKey;

  auto array = doc.createNestedArray("data");

  for (size_t i = 0; i < _datas.size() && i < count; i++) {

    auto data = _datas.at(i);

    auto jdata       = array.createNestedObject();
    jdata["created"] = data.time;
    jdata["time"]    = 1;
    jdata["d1"]      = String(data.temperature, 2);
    jdata["d2"]      = String(data.humidity, 1);
    jdata["d3"]      = String(data.pressure, 2);
  }

  serializeJson(doc, *retval);
}

/**
 * @brief _datas を Ambient 用の形式で addr に送信
 * 
 * @param addr 宛先の HTTP アドレス
 * @param writeKey ライトキー
 * @param maxLength 1リクエストに含める envdata_t の最大数
 * @return size_t 実際に送られた envdata_t の個数
 */
size_t postEnvdatas(const String &addr, const String &writeKey, const size_t maxLength) {

  String json;
  size_t count = std::min(_datas.size(), maxLength);

  serializeEnvDatas(count, writeKey, &json);

  if (httpPost(addr, FPSTR(MIME_APPLICATION_JSON), json) == HTTP_CODE_OK)
    return count;
  return 0;
}