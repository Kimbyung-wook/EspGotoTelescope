#include "mock_ui.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace sim {

namespace {

const double kRadToDeg = 57.295779513082320876798154814105;

const char* const kMainMenu[MockUi::kMainItems] = {
  "1RA Speed",
  "2Status-GPS",
  "3Status-Acc/Gyro",
  "4Status-Mag/Alt",
  "5Status-IMUCalib",
  "6Do IMU Calib.",
  "7Att.Estimator",
  "8Health",
  "9Axis for Moon",
  "Sleep",
};

// Screen reached from each main menu item.
const MockUi::Screen kMainTarget[MockUi::kMainItems] = {
  MockUi::kRaSpeed,      MockUi::kStatusGps,     MockUi::kStatusAccGyro,
  MockUi::kStatusMagAlt, MockUi::kStatusImuCalib, MockUi::kDoImuCalib,
  MockUi::kAttEstimator, MockUi::kHealth,        MockUi::kMoonAxis,
  MockUi::kSleep,
};

const char* const kCalibNames[4] = {
  "Acc. Bias Calib", "Acc. Scale Calib", "Gyro Bias Calib", "Gyro Scale Calib",
};

// Placeholder estimator tuning values (no estimator in the mock).
const double kEstR[3] = {0.5, 0.5, 2.0};
const double kEstQ[3] = {0.001, 0.001, 0.01};
const double kEstK[3] = {0.0447, 0.0447, 0.0707};

// Print into one display line; truncates at 21 chars like DISP_LINE_ROW.
void put(Frame& f, int line, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::vsnprintf(f.lines[line], kCols + 1, fmt, ap);
  va_end(ap);
}

double rollDeg(const ImuState& m) { return std::atan2(m.ay, m.az) * kRadToDeg + 0.0; }  // +0.0: no "-0.0000"

double pitchDeg(const ImuState& m) {
  double g = std::sqrt(m.ax * m.ax + m.ay * m.ay + m.az * m.az);
  if (g <= 0.0) return 0.0;
  double r = -m.ax / g;
  if (r > 1.0) r = 1.0;
  if (r < -1.0) r = -1.0;
  return std::asin(r) * kRadToDeg + 0.0;
}

double headingDeg(const ImuState& m) {
  double h = std::atan2(-m.my, m.mx) * kRadToDeg;
  if (h < 0.0) h += 360.0;
  return h + 0.0;
}

}  // namespace

void ListCursor::move(int delta) {
  sel += delta;
  if (sel < 0) sel = 0;
  if (sel > count - 1) sel = count - 1;
  if (sel < first) first = sel;
  if (sel > first + rows - 1) first = sel - rows + 1;
}

void MockUi::begin(const BootConfig& boot, const SensorState&) {
  boot_ = boot;
  screen_ = kBoot;
  boot_frame_ = 0;
  have_count_ = false;
  blink_ = '>';
  main_.reset(kMainItems);
  ra_.reset(3);
  calib_.reset(kCalibItems);
  att_.reset(kAttItems);
  ra_cmd_ = 0.0;
  motor_ = MotorOutput();
  bwx_ = bwy_ = bwz_ = 0.0;
  bax_ = bay_ = baz_ = 0.0;
}

void MockUi::frame(unsigned long now_ms, const InputSnapshot& input,
                   const SensorState& sensors, Frame& out, MotorOutput& motor) {
  int delta = have_count_ ? input.count - last_count_ : 0;
  last_count_ = input.count;
  have_count_ = true;

  if (screen_ == kBoot) {
    // Inputs during boot are dropped, as the firmware is still in setup().
    // The last boot line appears at frame 7*kBootLineFrames and is held for
    // kBootHoldFrames frames.
    if (boot_frame_ >= 7 * kBootLineFrames + kBootHoldFrames) screen_ = kMain;
  } else {
    handleRotation(delta);
    if (input.clicked()) handleClick(now_ms);
  }
  if (screen_ == kCalibGyroBias) updateGyroBias(now_ms, sensors);

  out.clear();
  render(now_ms, sensors, out);
  motor = motor_;
  if (screen_ == kBoot) boot_frame_++;

  // Cursor blinks: '>' and '-' on alternate frames (10 Hz loop -> 5 Hz blink).
  blink_ = (blink_ == '>') ? '-' : '>';
}

void MockUi::handleRotation(int delta) {
  if (delta == 0) return;
  switch (screen_) {
    case kMain: main_.move(delta); break;
    case kRaSpeed: ra_.move(delta); break;
    case kRaSpeedEdit:
      ra_cmd_ += 1.0 * delta;   // one tick = 1 (duty step)
      if (ra_cmd_ > 255.0) ra_cmd_ = 255.0;
      if (ra_cmd_ < -255.0) ra_cmd_ = -255.0;
      updateMotor();
      break;
    case kDoImuCalib: calib_.move(delta); break;
    case kAttEstimator: att_.move(delta); break;
    default: break;   // info screens ignore rotation
  }
}

void MockUi::handleClick(unsigned long now_ms) {
  switch (screen_) {
    case kMain: {
      Screen next = kMainTarget[main_.sel];
      if (next == kRaSpeed) ra_.reset(3);
      if (next == kDoImuCalib) calib_.reset(kCalibItems);
      if (next == kAttEstimator) att_.reset(kAttItems);
      screen_ = next;
      break;
    }
    case kRaSpeed:
      if (ra_.sel == 0) screen_ = kRaSpeedEdit;
      else if (ra_.sel == 2) screen_ = kMain;   // Exit
      break;                                   // RRate: read-only
    case kRaSpeedEdit:
      screen_ = kRaSpeed;   // confirm
      break;
    case kDoImuCalib:
      switch (calib_.sel) {
        case 0: screen_ = kCalibAccBias; break;
        case 1: screen_ = kCalibAccScale; break;
        case 2: screen_ = kCalibGyroBias; calib_start_ms_ = now_ms; break;
        case 3: screen_ = kCalibGyroScale; break;
        default: screen_ = kMain; break;   // Quit
      }
      break;
    case kCalibAccBias:
    case kCalibAccScale:
    case kCalibGyroBias:
    case kCalibGyroScale:
      screen_ = kDoImuCalib;   // cursor stays on the item we came from
      break;
    case kAttEstimator:
      if (att_.sel == kAttItems - 1) screen_ = kMain;   // Exit
      break;
    default:
      screen_ = kMain;   // info screens and Sleep: click returns
      break;
  }
}

void MockUi::updateMotor() {
  int duty = (int)std::fabs(ra_cmd_);
  if (duty > 255) duty = 255;
  motor_.pwm = duty;
  if (ra_cmd_ == 0.0) { motor_.in1 = 0; motor_.in2 = 0; }
  else if (ra_cmd_ > 0.0) { motor_.in1 = 0; motor_.in2 = 1; }
  else { motor_.in1 = 1; motor_.in2 = 0; }
}

void MockUi::updateGyroBias(unsigned long now_ms, const SensorState& s) {
  unsigned long t = now_ms - calib_start_ms_;
  const ImuState& m = s.imu;
  if (t < 1000) {
    bwx_ = m.wx; bwy_ = m.wy; bwz_ = m.wz;
  } else if (t < 16000) {
    double a = (t < 6000) ? 0.3 : 0.01;
    bwx_ = (1.0 - a) * bwx_ + a * m.wx;
    bwy_ = (1.0 - a) * bwy_ + a * m.wy;
    bwz_ = (1.0 - a) * bwz_ + a * m.wz;
  }
}

void MockUi::renderBoot(Frame& out) {
  const char* lines[8] = {
    "1. Display is ON",
    boot_.imu_ok ? "2. IMU is ON" : ">> IMU failed",
    "Acc +-16G",
    "Gyro 250 dps",
    "Mag 10 Hz",
    boot_.gps_ok ? "3. GPS is ON" : ">> GPS is timeout",
    "4. Buttons are ON",
    "5. GPIO/PWM is ON",
  };
  int shown = boot_frame_ / kBootLineFrames + 1;
  if (shown > 8) shown = 8;
  for (int i = 0; i < shown; i++) put(out, i, "%s", lines[i]);
  // setup() leaves PWM at 120 with both bridge inputs LOW (motor coasting).
  if (shown >= 8) { motor_.pwm = 120; motor_.in1 = 0; motor_.in2 = 0; }
}

void MockUi::renderStatusBar(const SensorState& s, Frame& out) {
  const GpsState& g = s.gps;
  put(out, 0, "T %02d:%02d:%02d.%1d V%5.2f", g.hour, g.minute, g.seconds,
      g.milliseconds / 100, 12.60);
}

void MockUi::render(unsigned long now_ms, const SensorState& s, Frame& out) {
  if (screen_ == kBoot) { renderBoot(out); return; }
  renderStatusBar(s, out);

  const ImuState& m = s.imu;
  const GpsState& g = s.gps;
  char ns = g.latitude >= 0 ? 'N' : 'S';
  char ew = g.longitude >= 0 ? 'E' : 'W';

  switch (screen_) {
    case kMain:
      put(out, 1, "Main Menu");
      for (int r = 0; r < 6; r++) {
        int item = main_.first + r;
        put(out, 2 + r, "%c%s", cur(item == main_.sel), kMainMenu[item]);
      }
      break;

    case kRaSpeed:
    case kRaSpeedEdit: {
      bool edit = screen_ == kRaSpeedEdit;
      put(out, 1, edit ? "*Set RA Speed" : "*%s", kMainMenu[0]);
      put(out, 2, "%cRRcmd %9.4fDEG", edit ? blink_ : cur(ra_.sel == 0), ra_cmd_);
      put(out, 3, "%cRRate %9.4fDEG", edit ? ' ' : cur(ra_.sel == 1), m.wx);
      put(out, 4, "%cExit", edit ? ' ' : cur(ra_.sel == 2));
      put(out, 5, " ROL %9.4f   DEG", rollDeg(m));
      put(out, 6, " LAT %c%10.6fDEG", ns, std::fabs(g.latitude));
      put(out, 7, " PIT %9.4f   DEG", pitchDeg(m));
      break;
    }

    case kStatusGps: {
      const char* fix = g.fixquality == 0 ? "NoFix"
                      : g.fixquality == 1 ? "GPS"
                      : g.fixquality == 2 ? "DGPS" : "Q?";
      put(out, 1, "*%s", kMainMenu[1]);
      put(out, 2, "Date 20%02d/%02d/%02d", g.year, g.month, g.day);
      put(out, 3, "Time   %02d:%02d:%02d.%03d", g.hour, g.minute, g.seconds, g.milliseconds);
      put(out, 4, "%-5s Sat %2d", fix, g.satellites);
      put(out, 5, "");
      put(out, 6, " %c %12.8f DEG", ns, std::fabs(g.latitude));
      put(out, 7, " %c %12.8f DEG", ew, std::fabs(g.longitude));
      break;
    }

    case kStatusAccGyro:
      put(out, 1, "*%s", kMainMenu[2]);
      put(out, 2, "Ax    %9.4f mpss", m.ax);
      put(out, 3, "Ay    %9.4f mpss", m.ay);
      put(out, 4, "Az    %9.4f mpss", m.az);
      put(out, 5, "Wx    %9.4f dps", m.wx);
      put(out, 6, "Wy    %9.4f dps", m.wy);
      put(out, 7, "Wz    %9.4f dps", m.wz);
      break;

    case kStatusMagAlt:
      put(out, 1, "*%s", kMainMenu[3]);
      put(out, 2, "Mx    %9.4f uT", m.mx);
      put(out, 3, "My    %9.4f uT", m.my);
      put(out, 4, "Mz    %9.4f uT", m.mz);
      put(out, 5, "HEAD  %9.4f DEG", headingDeg(m));
      put(out, 6, "Temp  %9.4f C", m.temperature);
      break;

    case kStatusImuCalib:
      put(out, 1, "*%s", kMainMenu[4]);
      put(out, 2, "bAx   %9.4f mpss", bax_);
      put(out, 3, "bAy   %9.4f mpss", bay_);
      put(out, 4, "bAz   %9.4f mpss", baz_);
      put(out, 5, "bWx   %9.4f dps", bwx_);
      put(out, 6, "bWy   %9.4f dps", bwy_);
      put(out, 7, "bWz   %9.4f dps", bwz_);
      break;

    case kDoImuCalib:
      put(out, 1, "*%s", kMainMenu[5]);
      for (int i = 0; i < 4; i++) put(out, 2 + i, "%c%s", cur(calib_.sel == i), kCalibNames[i]);
      put(out, 6, "");
      put(out, 7, "%cQuit", cur(calib_.sel == 4));
      break;

    case kAttEstimator: {
      put(out, 1, "*%s", kMainMenu[6]);
      const char* names[12] = {"Roll ", "PITCH", "YAW  ", "R_R  ", "R_P  ", "R_Y  ",
                               "Q_R  ", "Q_P  ", "Q_Y  ", "K_R  ", "K_P  ", "K_Y  "};
      double vals[12] = {rollDeg(m), pitchDeg(m), headingDeg(m),
                         kEstR[0], kEstR[1], kEstR[2], kEstQ[0], kEstQ[1], kEstQ[2],
                         kEstK[0], kEstK[1], kEstK[2]};
      for (int r = 0; r < 6; r++) {
        int item = att_.first + r;
        char c = cur(item == att_.sel);
        if (item == kAttItems - 1) put(out, 2 + r, "%cExit", c);
        else if (item < 3) put(out, 2 + r, "%c%s %9.4fDEG", c, names[item], vals[item]);
        else put(out, 2 + r, "%c%s %9.4f", c, names[item], vals[item]);
      }
      break;
    }

    case kHealth:
      // Mock timings [us]; a real backend would report measured values.
      put(out, 1, "*%s", kMainMenu[7]);
      put(out, 2, "Loop %6d/%5.1fHz", 100000, 10.0);
      put(out, 3, "MAIN %6d", 24300);
      put(out, 4, "DISP %6d", 21800);
      put(out, 5, "GPS  %6d", 650);
      put(out, 6, "IMU  %6d", 1250);
      put(out, 7, "EST  %6d", 0);
      break;

    case kMoonAxis:
      // Moon RA/Dec are placeholders until the ephemeris exists.
      put(out, 1, "*%s", kMainMenu[8]);
      put(out, 2, "Roll  %9.4f DEG", rollDeg(m));
      put(out, 3, "Pitch %9.4f DEG", pitchDeg(m));
      put(out, 4, "Yaw   %9.4f DEG", headingDeg(m));
      put(out, 5, "-- Moon --------");
      put(out, 6, "R.A.  %9.4f DEG", 37.0013);
      put(out, 7, "DEC.  %9.4f DEG", -12.0014);
      break;

    case kSleep:
      put(out, 1, "(Sleep)");
      break;

    case kCalibAccBias:
    case kCalibAccScale:
    case kCalibGyroScale: {
      int idx = screen_ == kCalibAccBias ? 0 : screen_ == kCalibAccScale ? 1 : 3;
      put(out, 1, "*%s", kCalibNames[idx]);
      put(out, 2, "Not implemented");
      put(out, 4, "Click to return");
      break;
    }

    case kCalibGyroBias: {
      unsigned long t = now_ms - calib_start_ms_;
      const char* phase = t < 1000 ? "Ready" : t < 6000 ? "ing.1" : t < 16000 ? "ing.2" : "Done.";
      put(out, 1, "Calib Gyro Bias %s", phase);
      put(out, 2, "Wx %8.5f->%8.5f", m.wx, m.wx - bwx_);
      put(out, 3, "Wy %8.5f->%8.5f", m.wy, m.wy - bwy_);
      put(out, 4, "Wz %8.5f->%8.5f", m.wz, m.wz - bwz_);
      put(out, 5, "bWx   %9.4f dps", bwx_);
      put(out, 6, "bWy   %9.4f dps", bwy_);
      put(out, 7, "bWz   %9.4f dps", bwz_);
      break;
    }

    default:
      put(out, 1, "ERROR");
      break;
  }
}

}  // namespace sim
