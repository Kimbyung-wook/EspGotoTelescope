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
