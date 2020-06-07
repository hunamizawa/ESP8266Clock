#ifndef MAX7219Display_Panes_H_
#define MAX7219Display_Panes_H_

#include <Arduino.h>

enum class Panes : uint8_t {
  DATE_TIME,
  TEMP_HUMI_TIME,
  PRES_TIME,
  TIME,
  IP_ADDR,
  WELCOME,
  REQUIRE_SETTING,
  SYNCING_TIME,
  CONNECT_FAILED,
  INVALID,
};

enum class OverridePanes : uint8_t {
  NORMAL,
  TEST,
  OFF,
};

String toString(const Panes v);

String toString(const OverridePanes v);

Panes toPanes(const String v);

OverridePanes toOverridePanes(const String v);

bool isValid(const Panes pane);

bool isValidExternalUse(const Panes pane);

#endif // MAX7219Display_Panes_H_