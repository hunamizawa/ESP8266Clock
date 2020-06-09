/**
 * @file ClockSetting.h
 */

#ifndef ClockSetting_H_
#define ClockSetting_H_

#include "display/Brightness.h"
#include "display/MyBuffer.h"
#include "setting.h"
#include <Arduino.h>
#include <ArduinoJson.h>

// 設定の永続化には、あえて JSON を用いた
// 将来、設定項目が増えた時に
// f.write((void*)setting, sizeof(setting)) でシリアライズするより
// 後方互換性を取りやすいと思った
// JSON は human-readable だからデバッグしやすいし

// デフォルト値の定義
static constexpr Panes                   DEFAULT_PANE                    = Panes::DATE_TIME;
static constexpr OverridePanes           DEFAULT_OVERRIDE_PANE           = OverridePanes::NORMAL;
static constexpr int8_t                  DEFAULT_BRIGHTNESS_MANUAL_VALUE = -1;
static constexpr std::array<uint16_t, 6> DEFAULT_BRIGHTNESS_THRESHOLDS   = {1024, 360, 270, 200, 160, 120};
static constexpr uint16_t                DEFAULT_BRIGHTNESS_HYSTERESIS   = 10;
static constexpr uint16_t                DEFAULT_ELEV                    = 0;
static constexpr bool                    DEFAULT_USE_AMBIENT             = false;
static constexpr unsigned int            DEFAULT_AMBIENT_CHANNELID       = 0;
static constexpr char                    DEFAULT_AMBIENT_WRITEKEY[]      = "";
static constexpr bool                    DEFAULT_USE_CUSTOM_SERVER       = false;
static constexpr char                    DEFAULT_CUSTOM_SERVER_ADDR[]    = "";
static constexpr brightness_setting_t    DEFAULT_BRIGHTNESS              = {
    DEFAULT_BRIGHTNESS_MANUAL_VALUE,
    DEFAULT_BRIGHTNESS_THRESHOLDS,
    DEFAULT_BRIGHTNESS_HYSTERESIS};

// #define DEFAULT_BRIGHTNESS_THRESHOLDS { 1024, 360, 270, 200, 160, 120 }

// Copy a 1D array to a JsonArray
template <typename T, size_t N>
static inline bool copyArray(std::array<T, N> src, ARDUINOJSON_NAMESPACE::ArrayRef dst) {
  return copyArray(src.data(), src.size(), dst);
}

// Copy a JsonArray to a 1D array
template <typename T, size_t N>
static inline size_t copyArray(ARDUINOJSON_NAMESPACE::ArrayConstRef src, std::array<T, N> dst) {
  return copyArray(src, dst.data(), dst.size());
}

/**
 * @brief 設定を格納する class
 * 
 */
class ClockSetting {
  // struct のように使ってほしいのでメンバ変数は全部 public
public:
  Panes                pane;
  OverridePanes        override_pane;
  brightness_setting_t brightness;
  String               tzarea;
  String               tzcity;
  std::vector<String>  ntp;
  uint16_t             elev;
  bool                 use_ambient;
  unsigned int         ambient_channelid;
  String               ambient_writekey;
  bool                 use_custom_server;
  String               custom_server_addr;

  /**
   * @brief このクラスをシリアライズする
   * 
   * @param retval JSON 文字列が返る
   */
  template <class T>
  void serialize(T &retval) {
    const size_t        capacity = JSON_ARRAY_SIZE(3) + JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(12);
    DynamicJsonDocument doc(capacity);

    doc["pane"]          = toString(pane);
    doc["override_pane"] = toString(override_pane);

    auto brightness_j            = doc.createNestedObject("brightness");
    brightness_j["manual_value"] = brightness.manual_value;
    brightness_j["hysteresis"]   = brightness.hysteresis;

    auto thresholds_j = brightness_j.createNestedArray("thresholds");
    copyArray(brightness.thresholds, thresholds_j);

    doc["tzarea"] = tzarea;
    doc["tzcity"] = tzcity;

    auto ntp_j = doc.createNestedArray("ntp");
    for (auto &&i : ntp)
      ntp_j.add(i);

    doc["elev"]               = elev;
    doc["use_ambient"]        = use_ambient;
    doc["ambient_channelid"]  = ambient_channelid;
    doc["ambient_writekey"]   = ambient_writekey;
    doc["use_custom_server"]  = use_custom_server;
    doc["custom_server_addr"] = custom_server_addr;

    serializeJson(doc, retval);
  }

private:
  template <class TJson, class TMember>
  TMember getOrDefault(const TJson &json, const char *member, TMember defaultValue) {
    return json.containsKey(member) ? json[member].template as<TMember>() : defaultValue;
  }

  /**
   * @brief JSON 文字列をデシリアライズして、このオブジェクトのメンバ変数に読み込む
   * 
   * @param json JSON 文字列
   */
public:
  void deserialize(const String &json) {
    const size_t        capacity = JSON_ARRAY_SIZE(3) + JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(12) + 350;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, json);

    pane = toPanes(getOrDefault(doc, "pane", toString(DEFAULT_PANE)));
    // override_pane はデシリアライズしない
    override_pane      = DEFAULT_OVERRIDE_PANE;

    auto brightness_j = doc["brightness"];
    if (brightness_j) {
      auto thresholds = brightness_j["thresholds"].as<JsonArray>();
      if (thresholds && thresholds.size() == brightness.thresholds.size()) {
        copyArray(thresholds, brightness.thresholds);
      } else {
        brightness.thresholds = DEFAULT_BRIGHTNESS_THRESHOLDS;
      }

      brightness.manual_value = getOrDefault(brightness_j, "manual_value", DEFAULT_BRIGHTNESS_MANUAL_VALUE);
      brightness.hysteresis   = getOrDefault(brightness_j, "hysteresis", DEFAULT_BRIGHTNESS_HYSTERESIS);
      
    } else {
      brightness = DEFAULT_BRIGHTNESS;
    }

    tzarea             = getOrDefault(doc, "tzarea", DEFAULT_TZAREA);
    tzcity             = getOrDefault(doc, "tzcity", DEFAULT_TZCITY);

    ntp.clear();
    auto ntp_j = doc["ntp"].as<JsonArray>();
    if (ntp_j) {
      for (auto &&i : ntp_j) {
        if (i.is<const char *>())
          ntp.push_back(i.as<const char *>());
      }
    } else {
      ntp.push_back(DEFAULT_NTP_SERVER);
    }

    elev               = getOrDefault(doc, "elev", DEFAULT_ELEV);
    use_ambient        = getOrDefault(doc, "use_ambient", DEFAULT_USE_AMBIENT);
    ambient_channelid  = getOrDefault(doc, "ambient_channelid", DEFAULT_AMBIENT_CHANNELID);
    ambient_writekey   = getOrDefault(doc, "ambient_writekey", DEFAULT_AMBIENT_WRITEKEY);
    use_custom_server  = getOrDefault(doc, "use_custom_server", DEFAULT_USE_CUSTOM_SERVER);
    custom_server_addr = getOrDefault(doc, "custom_server_addr", DEFAULT_CUSTOM_SERVER_ADDR);
  }

  void resetToDefault() {
    pane               = DEFAULT_PANE;
    override_pane      = DEFAULT_OVERRIDE_PANE;
    brightness         = DEFAULT_BRIGHTNESS;
    tzarea             = DEFAULT_TZAREA;
    tzcity             = DEFAULT_TZCITY;
    ntp                = {DEFAULT_NTP_SERVER};
    elev               = DEFAULT_ELEV;
    use_ambient        = DEFAULT_USE_AMBIENT;
    ambient_channelid  = DEFAULT_AMBIENT_CHANNELID;
    ambient_writekey   = DEFAULT_AMBIENT_WRITEKEY;
    use_custom_server  = DEFAULT_USE_CUSTOM_SERVER;
    custom_server_addr = DEFAULT_CUSTOM_SERVER_ADDR;
  }
};

// #undef DEFAULT_BRIGHTNESS_THRESHOLDS

#endif // ClockSetting_H_