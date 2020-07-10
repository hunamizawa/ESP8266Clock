#include "MyBuffer.h"
#include "../main.h"
#include "MyGraphics.h"
#include <MAX7219Display.h>
#include <string>
using std::u16string;

static constexpr suseconds_t HALF_OF_SEC_US = 500000; // 0.5秒

static const std::array<char16_t, 7> wd = {u'日', u'月', u'火', u'水', u'木', u'金', u'土'};

void MyBuffer::timeRow2(const struct tm &tm) {
  if (tm.tm_hour >= 10)
    this->writeInteger<4, 10>(tm.tm_hour / 10, 0, 0, 6, 4); // 時
  this->writeInteger<5, 10>(tm.tm_hour % 10, 0, 5, 6, 5);   // 時
  this->turnDot(true, 11, 8);                               // コロン
  this->turnDot(true, 11, 13);                              // コロン
  this->writeInteger<5, 10>(tm.tm_min, 2, 13, 6, 11);       // 分
  this->writeInteger<3, 5>(tm.tm_sec, 2, 25, 11, 7);        // 秒
}

bool MyBuffer::isRequireUpdate(const struct tm &tm, suseconds_t us, const envdata_t &envData, IPAddress *addr) const {

  if ((_override_pane == OverridePanes::OFF) != (_prev_override_pane == OverridePanes::OFF))
    return true;

  if (_pane != _prev_pane)
    return true;

  auto tm_changed = _prev_tm != tm;

  if (_override_pane == OverridePanes::OFF)
    return tm_changed;

  auto envData_changed = envData != _prev_envData;

  bool addr_changed;
  if (addr && addr->isSet())
    addr_changed = *addr != _prev_addr;
  else
    addr_changed = (addr && addr->isSet()) != _prev_addr.isSet();

  switch (_pane) {
  case Panes::SYNCING_TIME: /* 同期中... */
  case Panes::TIME:         /* 時刻のみ、秒なし */
    return tm_changed || (_prev_us < HALF_OF_SEC_US && HALF_OF_SEC_US <= us);

  case Panes::IP_ADDR: /* IPアドレス表示 */
    return addr_changed;

  case Panes::PRES_TIME:      /* 気圧＋時刻 */
  case Panes::TEMP_HUMI_TIME: /* 温度＋湿度＋時刻 */
    return tm_changed || envData_changed;

  case Panes::DATE_TIME: /* 日付＋時刻 */
    return tm_changed;

  default:
    return false;
  }
}

void MyBuffer::assignPrevValues(const struct tm &tm, suseconds_t us, const envdata_t &envData, IPAddress *addr) {

  _prev_pane          = _pane;
  _prev_override_pane = _override_pane;
  _prev_tm            = tm;
  _prev_us            = us;
  _prev_envData       = envData;

  if (addr && addr->isSet()) {
    _prev_addr = *addr;
  } else {
    _prev_addr = IPAddress();
  }
}

void MyBuffer::update(const struct tm &tm, suseconds_t us, const envdata_t &envData, IPAddress *addr) {

  assignPrevValues(tm, us, envData, addr);

  if (_override_pane == OverridePanes::OFF) {
    /* 画面オフ（フリーズしてないことを示すため 1 ドット点滅させる） */
    this->clearAll();
    this->turnDot((tm.tm_sec & 1) == 0, 0, 0);
    return;
  }

  switch (_pane) {
  case Panes::WELCOME:
    /* タイトル画面 */
    this->clearAll();
    this->write(ConstGraphics::welcome, 0, 0, 32);
    break;

  case Panes::REQUIRE_SETTING:
    /* 設定してください */
    this->clearAll();
    this->write(ConstGraphics::plz_setting, 0, 0, 32);
    break;

  case Panes::SYNCING_TIME:
    /* 同期中... */
    this->clearAll();

    this->write(ConstGraphics::douki_chu, 0, 8, 32);
    // 3点リーダをアニメーションさせる
    this->turnDot((tm.tm_sec & 1) == 0 || us < HALF_OF_SEC_US, 27, 15);
    this->turnDot((tm.tm_sec & 1) == 1 || us >= HALF_OF_SEC_US, 29, 15);
    this->turnDot((tm.tm_sec & 1) == 1 || us < HALF_OF_SEC_US, 31, 15);
    break;

  case Panes::CONNECT_FAILED:
    /* Wi-Fi 接続不可 */
    this->write(ConstGraphics::con_fail, 0, 0, 32);
    break;

  case Panes::IP_ADDR:
    /* IPアドレス表示 */
    this->clearAll();

    if (addr && addr->isV4()) {
      this->writeInteger<3, 7>((*addr)[0], 0, 0, 0, 11);  // 第1オクテット
      this->turnDot(true, 12, 6);                         // 小数点
      this->writeInteger<3, 7>((*addr)[1], 0, 14, 0, 11); // 第2オクテット
      this->turnDot(true, 26, 6);                         // 小数点
      this->writeInteger<3, 7>((*addr)[2], 0, 6, 8, 11);  // 第3オクテット
      this->turnDot(true, 18, 14);                        // 小数点
      this->writeInteger<3, 7>((*addr)[3], 0, 20, 8, 11); // 第4オクテット
    } else {
      // IPv6 は非対応...のはず
      this->writeString<3, 7>(u"---.---.", 14, 0);
      this->writeString<3, 7>(u"---.---", 20, 8);
    }
    break;

  case Panes::PRES_TIME:
    /* 気圧＋時刻 */
    this->clearAll();

    if (!envData.isValid()) {
      this->writeString<3, 5>(u" ---.-", -1, 0);
    } else {
      this->writeReal<3, 5>(envData.pressure, 1, -1, 0, 21);
    }
    this->writeString<3, 5>(u"hPa", 21, 0);

    timeRow2(tm);
    break;

  case Panes::TEMP_HUMI_TIME:
    /* 温度＋湿度＋時刻 */
    this->clearAll();

    // 温度表示
    if (!envData.isValid()) {
      this->writeString<3, 5>(u"--.-", 0, 0);
    } else if (envData.temperature < -40.0f) {
      // 低すぎ
      this->writeString<3, 5>(u"Lo", 3, 0);
    } else if (envData.temperature > 85.0f) {
      // 高すぎ
      this->writeString<3, 5>(u"Hi", 3, 0);
    } else {
      this->writeReal<3, 5>(envData.temperature, 1, 0, 0, 13);
    }
    this->writeString<4, 5>(u"℃", 14, 0);

    // 湿度表示
    if (!envData.isValid())
      this->writeString<3, 5>(u"--", 19, 0);
    else if (envData.humidity <= 0.0f)
      this->writeString<3, 5>(u"Lo", 19, 0);
    else if (envData.humidity >= 100.0f)
      this->writeString<3, 5>(u"RH", 19, 0);
    else
      this->writeReal<3, 5>(envData.humidity, 0, 19, 0, 7);
    this->writeString<5, 5>(u"%", 27, 0);

    timeRow2(tm);
    break;

  case Panes::TIME:
    /* 時刻のみ、秒なし */
    this->clearAll();

    if (tm.tm_hour >= 10)
      this->writeInteger<6, 16>(tm.tm_hour / 10, 0, 0, 0, 6); // 時
    this->writeInteger<7, 16>(tm.tm_hour % 10, 0, 7, 0, 7);   // 時
    this->turnDot(us < HALF_OF_SEC_US, 15, 4);                // コロン
    this->turnDot(us < HALF_OF_SEC_US, 15, 11);               // コロン
    this->writeInteger<7, 16>(tm.tm_min, 2, 17, 0, 15);       // 分
    break;

  default: /* DATE_TIME */
    /* 日付＋時刻 */
    this->clearAll();

    this->writeInteger<4, 5>(tm.tm_mon + 1, 0, -2, 0, 9); // 月
    this->writeString<5, 5>(u"/", 8, 0);                  //
    this->writeInteger<4, 5>(tm.tm_mday, 0, 14, 0, 9);    // 日
    this->writeChar<7, 7>(wd.at(tm.tm_wday), 25, 0);      // 曜日

    timeRow2(tm);
    break;
  }
}

void MyBuffer::setPane(const Panes pane) {
  assert_debug(isValid(pane));
  if (isValid(pane))
    _pane = pane;
}

Panes MyBuffer::getPane() const {
  return _pane;
}

void MyBuffer::setOverridePane(const OverridePanes pane) {
  _override_pane = pane;
}

OverridePanes MyBuffer::getOverridePane() const {
  return _override_pane;
}
