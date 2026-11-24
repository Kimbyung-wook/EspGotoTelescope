#pragma once
#ifndef _GPS_LINK_H_
#define _GPS_LINK_H_

// GPS link is alive when an NMEA sentence arrived within timeout_ms.
// last_recv_ms == 0 means nothing has been received yet.
inline bool gpsLinkAlive(unsigned long last_recv_ms, unsigned long now_ms,
                         unsigned long timeout_ms) {
  return (last_recv_ms != 0) && (now_ms - last_recv_ms <= timeout_ms);
}

#include <stdio.h>
#include "../astro/astro.h"

// Adafruit_GPS gives a 2-digit year (26 = 2026). Returns false when the GPS has
// not delivered a date yet (year/month/day are 0 before the first RMC sentence).
inline bool utcFromGps(int yy, int month, int day, int hour, int minute, int seconds, int ms,
                       astro::UtcTime& out) {
  if (month < 1 || month > 12 || day < 1 || day > 31 || yy < 0 || yy > 99) return false;
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || seconds < 0 || seconds > 60) return false;
  out.year = 2000 + yy;
  out.month = month;
  out.day = day;
  out.hour = hour;
  out.minute = minute;
  out.second = seconds + ms / 1000.0;
  return true;
}

// Fix quality text for the status screen : NoFix / GPS / DGPS / Q<n>
inline void gpsFixText(int fixquality, char* out, size_t size) {
  switch (fixquality) {
    case 0:  snprintf(out, size, "NoFix"); break;
    case 1:  snprintf(out, size, "GPS");   break;
    case 2:  snprintf(out, size, "DGPS");  break;
    default: snprintf(out, size, "Q%d", fixquality); break;
  }
}

#endif // _GPS_LINK_H_
