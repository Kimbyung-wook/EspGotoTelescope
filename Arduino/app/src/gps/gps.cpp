#include "gps.h"
#include "gps_link.h"

Gps::Gps(HardwareSerial* port) : device_(port) {}

bool Gps::begin() {
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's (some use 4800)
  delay(1000);
  device_.begin(9600);
  device_.sendCommand(PMTK_SET_BAUD_115200);
  device_.sendCommand(PMTK_SET_BAUD_115200);
  delay(200);
  device_.sendCommand(PMTK_SET_BAUD_115200);
  delay(200);

  device_.begin(115200);
  device_.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  device_.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
  delay(200);

  // Wait for the first valid NMEA sentence
  const unsigned long start = millis();
  while (true) {
    device_.read();
    if (device_.newNMEAreceived() && device_.parse(device_.lastNMEA())) return true;
    if (millis() - start > LINK_TIMEOUT_MS) return false;
    yield();
  }
}

bool Gps::read() {
  read_time_.start();
  char c = 0;
  do {
    c = device_.read();
    if (device_.newNMEAreceived()) {
      last_recv_ms_ = millis();
      device_.parse(device_.lastNMEA());     // a broken sentence is just skipped
    }
  } while (c);
  link_alive_ = gpsLinkAlive(last_recv_ms_, millis(), LINK_TIMEOUT_MS);
  read_time_.stop();
  return link_alive_;
}

GpsRaw Gps::data() const {
  GpsRaw g;
  g.year         = device_.year;
  g.month        = device_.month;
  g.day          = device_.day;
  g.hour         = device_.hour;
  g.minute       = device_.minute;
  g.seconds      = device_.seconds;
  g.milliseconds = device_.milliseconds;
  g.fixquality   = device_.fixquality;
  g.satellites   = device_.satellites;
  g.lat_hemi     = device_.lat;
  g.lon_hemi     = device_.lon;
  g.latitude     = ((double)device_.latitude_fixed)  * 1E-7;
  g.longitude    = ((double)device_.longitude_fixed) * 1E-7;
  g.altitude_m   = device_.altitude;
  return g;
}
