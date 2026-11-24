// Plain data shared between the protocol layer and a UI backend.
// No Arduino headers here: a backend that wraps the real firmware maps
// these values onto its own stubs (Rotary, GPS, IMU, motor pins).
#pragma once

namespace sim {

const int kLines = 8;        // SSD1306 128x64 with the 6x8 font -> 8 lines
const int kCols = 21;        // 21 characters per line
const int kFrameMs = 100;    // main loop runs at 10 Hz

// Fake GPS fix, same fields the firmware reads from Adafruit_GPS.
struct GpsState {
  int year = 26, month = 11, day = 21;      // year is 2-digit like Adafruit_GPS
  int hour = 12, minute = 0, seconds = 0, milliseconds = 0;
  double latitude = 37.5665;                // deg, + north
  double longitude = 126.9780;              // deg, + east
  int fixquality = 1;                       // 0 NoFix, 1 GPS, 2 DGPS
  int satellites = 8;
  double hdop = 1.0;                        // last GPS is kept only at HDOP <= 2 (DES-LOC-02)
  bool link = true;                         // false: no NMEA for 3 s (Status > GPS "NoLink")
};

// Fake IMU sample, already in the firmware's body frame and units
// (after the sign flips done in loop()): m/s^2, deg/s, uT, degC.
struct ImuState {
  double ax = 0.0, ay = 0.0, az = 9.80665;
  double wx = 0.0, wy = 0.0, wz = 0.0;
  double mx = 20.0, my = 0.0, mz = -40.0;
  double temperature = 22.0;
};

struct SensorState {
  GpsState gps;
  ImuState imu;
  double voltage = 12.60;   // supply [V] (A0), shown in the status bar
  int remote = 0;           // phone remote UI: 0 off, 1 AP waiting ('w'), 2 connected ('W')
};

// Which devices pretend to start correctly at boot.
struct BootConfig {
  bool imu_ok = true;
  bool gps_ok = true;
};

// What the rotary driver would report for one frame:
// count  = accumulated steps since boot (CW +, CCW -), like Rotary::count()
// clicks = clicks since the previous frame; Rotary::takeClick() is true when > 0
struct InputSnapshot {
  int count = 0;
  int clicks = 0;
  bool clicked() const { return clicks > 0; }
  // Phone clock sent by the remote UI page since the previous frame (DES-LOC-05)
  bool phone_time = false;
  unsigned long phone_unix_s = 0;
  int phone_ms = 0;
};

// H-bridge output for the RA motor.
struct MotorOutput {
  int in1 = 0;   // MOTOR_IN1 level (0/1)
  int in2 = 0;   // MOTOR_IN2 level (0/1)
  int pwm = 0;   // MOTOR_PWM duty 0..255
};

// One rendered frame: 8 text lines, each at most 21 chars + NUL.
struct Frame {
  char lines[kLines][kCols + 1];
  void clear() {
    for (int i = 0; i < kLines; i++) lines[i][0] = '\0';
  }
};

}  // namespace sim
