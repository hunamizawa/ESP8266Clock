#include "Panes.h"

String toString(const Panes v) {
  switch (v) {
  case Panes::DATE_TIME:
    return F("DATE_TIME");
  case Panes::TEMP_HUMI_TIME:
    return F("TEMP_HUMI_TIME");
  case Panes::PRES_TIME:
    return F("PRES_TIME");
  case Panes::TIME:
    return F("TIME");
  case Panes::IP_ADDR:
    return F("IP_ADDR");
  case Panes::WELCOME:
    return F("WELCOME");
  case Panes::REQUIRE_SETTING:
    return F("REQUIRE_SETTING");
  case Panes::SYNCING_TIME:
    return F("SYNCING_TIME");
  case Panes::CONNECT_FAILED:
    return F("CONNECT_FAILED");
  default:
    return F("INVALID");
  }
}

String toString(const OverridePanes v) {
  switch (v) {
  case OverridePanes::OFF:
    return F("OFF");
  case OverridePanes::TEST:
    return F("TEST");
  default:
    return F("NORMAL");
  }
}

Panes toPanes(const String v) {
  if (v == F("DATE_TIME"))
    return Panes::DATE_TIME;
  if (v == F("TEMP_HUMI_TIME"))
    return Panes::TEMP_HUMI_TIME;
  if (v == F("PRES_TIME"))
    return Panes::PRES_TIME;
  if (v == F("TIME"))
    return Panes::TIME;
  if (v == F("WELCOME"))
    return Panes::WELCOME;
  if (v == F("REQUIRE_SETTING"))
    return Panes::REQUIRE_SETTING;
  if (v == F("SYNCING_TIME"))
    return Panes::SYNCING_TIME;
  if (v == F("CONNECT_FAILED"))
    return Panes::CONNECT_FAILED;
  if (v == F("IP_ADDR"))
    return Panes::IP_ADDR;
  return Panes::INVALID;
}

OverridePanes toOverridePanes(const String v) {
  if (v == F("OFF"))
    return OverridePanes::OFF;
  if (v == F("TEST"))
    return OverridePanes::TEST;
  return OverridePanes::NORMAL;
}

bool isValid(const Panes pane) {
  return pane == Panes::DATE_TIME ||
         pane == Panes::TEMP_HUMI_TIME ||
         pane == Panes::PRES_TIME ||
         pane == Panes::TIME ||
         pane == Panes::WELCOME ||
         pane == Panes::REQUIRE_SETTING ||
         pane == Panes::SYNCING_TIME ||
         pane == Panes::CONNECT_FAILED ||
         pane == Panes::IP_ADDR;
}

bool isValidExternalUse(const Panes pane) {
  return pane == Panes::DATE_TIME ||
         pane == Panes::TEMP_HUMI_TIME ||
         pane == Panes::PRES_TIME ||
         pane == Panes::TIME ||
         pane == Panes::IP_ADDR;
}
