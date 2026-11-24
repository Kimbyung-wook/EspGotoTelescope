#pragma once
#ifndef _GPS_H_
#define _GPS_H_
#include <Adafruit_GPS.h>
#include "gps_data.h"
#include "../util/stopwatch_us.h"

/************************************/
// Adafruit Ultimate GPS (MTK) on a UART: 115200 baud, RMC + GGA at 5 Hz
/************************************/
class Gps {
public:
  static const unsigned long LINK_TIMEOUT_MS = 3000;   // no NMEA for this long: link lost

  explicit Gps(HardwareSerial* port);
  bool begin();                    // false when no NMEA sentence came within the timeout
  bool read();                     // drains the UART, parses every sentence; returns linkAlive()
  GpsRaw data() const;             // latest values
  bool linkAlive() const { return link_alive_; }
  unsigned long readUs() const { return read_time_.last(); }

private:
  Adafruit_GPS device_;
  StopwatchUs read_time_;
  bool link_alive_ = false;
  unsigned long last_recv_ms_ = 0;
};

#endif // _GPS_H_
