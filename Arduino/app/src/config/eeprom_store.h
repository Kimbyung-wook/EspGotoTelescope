#pragma once
#ifndef _CONFIG_EEPROM_STORE_H_
#define _CONFIG_EEPROM_STORE_H_
#include <EEPROM.h>
#include "settings.h"

/************************************/
// SettingsStore on the ESP8266 flash (EEPROM library : one 4 KB flash sector,
// a RAM copy of STORE_SIZE bytes). commit() erases and writes the sector only
// when a byte changed, so saving the same settings does not wear the flash.
// Board only (not built on the PC).
/************************************/
namespace config {

class EepromSettingsStore : public SettingsStore {
public:
  void begin() { EEPROM.begin(STORE_SIZE); }
  bool read(uint8_t* buf, size_t n) override {
    if (n > STORE_SIZE) return false;
    for (size_t i = 0; i < n; i++) buf[i] = EEPROM.read((int)i);
    return true;
  }
  bool write(const uint8_t* buf, size_t n) override {
    if (n > STORE_SIZE) return false;
    for (size_t i = 0; i < n; i++) EEPROM.write((int)i, buf[i]);
    return EEPROM.commit();
  }
};

} // namespace config

#endif // _CONFIG_EEPROM_STORE_H_
