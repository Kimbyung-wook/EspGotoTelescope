#pragma once
#ifndef _LOCATION_H_
#define _LOCATION_H_

/************************************/
// Where and when the observer is (docs/REQUIREMENTS.md DES-LOC-01..04)
//   position : GPS fix -> last GPS fix (saved) -> manual (saved), or manual only
//   time     : GPS (with a fix) -> phone clock (remote UI page, DES-LOC-05, not saved)
//              -> manual clock (set on the Time screen, not saved)
// Pure C++ (no Arduino calls).
/************************************/
namespace loc {

enum Mode { MODE_AUTO = 0, MODE_MANUAL = 1 };              // config::LocationMode
enum Source { SRC_NONE = 0, SRC_GPS, SRC_LAST, SRC_MANUAL };
enum TimeSource { TIME_NONE = 0, TIME_GPS, TIME_MANUAL, TIME_PHONE };

struct Position {
  double lat_deg = 0.0;   // north positive
  double lon_deg = 0.0;   // east positive
  float  alt_m = 0.0f;
};

const char* sourceText(int src);        // "GPS", "LAST", "MAN", "---"
const char* timeSourceText(int src);    // "GPS", "MAN", "PHN", "---"

// DES-LOC-01. Returns the Source used (SRC_NONE: no position, out unchanged)
int resolvePosition(int mode, bool gps_fix, const Position& gps,
                    bool last_valid, const Position& last,
                    bool manual_set, const Position& manual, Position& out);

// Great-circle distance [km]
double distanceKm(const Position& a, const Position& b);

// DES-LOC-02 : once per power-on, after a good fix held for 30 s, the last GPS
// position is replaced when there is none yet or it moved more than 1 km.
class LastGpsRecorder {
public:
  static const unsigned long HOLD_MS = 30000;
  static const int MIN_SATELLITES = 4;
  static constexpr double MOVE_KM = 1.0;

  // Call every frame. Returns true once, when 'gps' should become the last fix.
  bool update(unsigned long now_ms, int fixquality, int satellites, const Position& gps,
              bool last_valid, const Position& last);
  bool done() const { return done_; }
  void reset() { holding_ = false; done_ = false; }

private:
  bool holding_ = false;
  bool done_ = false;
  unsigned long since_ms_ = 0;
};

} // namespace loc

#endif // _LOCATION_H_
