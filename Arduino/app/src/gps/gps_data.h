#pragma once
#ifndef _GPS_DATA_H_
#define _GPS_DATA_H_

// GPS receiver output as it arrives (Adafruit_GPS fields, src/gps/gps.h)
struct GpsRaw {
  int  year = 0, month = 0, day = 0;        // 2-digit year (26 = 2026)
  int  hour = 0, minute = 0, seconds = 0, milliseconds = 0;
  int  fixquality = 0, satellites = 0;
  float hdop = 0.0f;                        // horizontal dilution of precision (GGA), 0: none yet
  char lat_hemi = ' ', lon_hemi = ' ';
  double latitude = 0.0, longitude = 0.0;   // [deg], signed (S/W negative)
  float  altitude_m = 0.0f;                 // above mean sea level
  bool   link = true;                       // NMEA arrived within Gps::LINK_TIMEOUT_MS
};

// A fix that can be used now: fixed, and still arriving (after a link loss the
// fields keep the last values)
inline bool gpsFixUsable(const GpsRaw& g) { return g.link && g.fixquality > 0; }

#endif // _GPS_DATA_H_
