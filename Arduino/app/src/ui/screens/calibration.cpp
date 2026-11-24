#include "calibration.h"

/************************************/
// Gyro calibration (DES-CAL-03)
/************************************/
void GyroCalibScreen::apply(config::Settings& cfg) const {
  for (int i = 0; i < 3; i++) cfg.gyro_bias[i] = calib_.bias(i);
}

void GyroCalibScreen::render(Frame& f, const UiContext& ctx, char blink) {
  using G = cal::GyroCalibration;
  const ImuData& d = ctx.tel.imu.raw;
  f.set(1, "*Gyro Calib");
  switch (calib_.state()) {
    case G::FAILED:
      f.set(2, "Shock detected.");
      f.set(3, "Keep the mount still");
      f.set(4, "and retry.");
      f.blank(5, 6);
      choice_.render(f, 7, "Retry", "Quit", blink);
      return;
    case G::RESULT:
      f.set(2, "Done. Apply?");
      break;
    case G::MEASURE:
      f.set(2, "Measuring %2d/%2d s", calib_.elapsedS(), calib_.totalS());
      break;
    default:
      f.set(2, "Keep still  0/%2d s", calib_.totalS());
      f.set(3, "Wx  %+10.5f dps", d.wx);
      f.set(4, "Wy  %+10.5f dps", d.wy);
      f.set(5, "Wz  %+10.5f dps", d.wz);
      f.blank(6);
      f.set(7, "%cCancel", blink);
      return;
  }
  f.set(3, "bWx %+10.5f dps", calib_.bias(0));
  f.set(4, "bWy %+10.5f dps", calib_.bias(1));
  f.set(5, "bWz %+10.5f dps", calib_.bias(2));
  f.set(6, "sd %5.3f %5.3f %5.3f", calib_.stddev(0), calib_.stddev(1), calib_.stddev(2));
  if (calib_.state() == G::RESULT) choice_.render(f, 7, "Apply", "Discard", blink);
  else                             f.set(7, "%cCancel", blink);
}

/************************************/
// Accelerometer six-position calibration (DES-CAL-04..06)
/************************************/
void AccelCalibScreen::apply(config::Settings& cfg) const {
  for (int i = 0; i < 3; i++) {
    cfg.accel_bias[i] = calib_.bias(i);
    cfg.accel_scale[i] = calib_.scale(i);
  }
}

void AccelCalibScreen::render(Frame& f, const UiContext& ctx, char blink) {
  using A = cal::AccelCalibration;
  const ImuData& d = ctx.tel.imu.raw;
  f.set(1, "*Accel Calib");
  if (calib_.state() == A::RESULT || calib_.state() == A::FAILED) {
    const bool ok = calib_.state() == A::RESULT;
    f.set(2, ok ? "Done. Apply?" : "Bad result. Retry");
    f.set(3, "X b%+8.4f s%6.4f", calib_.bias(0), calib_.scale(0));
    f.set(4, "Y b%+8.4f s%6.4f", calib_.bias(1), calib_.scale(1));
    f.set(5, "Z b%+8.4f s%6.4f", calib_.bias(2), calib_.scale(2));
    f.blank(6);
    if (ok) choice_.render(f, 7, "Apply", "Discard", blink);
    else    choice_.render(f, 7, "Retry", "Quit", blink);
    return;
  }
  const char* axis = A::stepName(calib_.step());
  f.set(2, "Step %d/%d %s up", calib_.step() + 1, A::STEPS, axis);
  switch (calib_.state()) {
    case A::SETTLE:  f.set(3, "Hold still..."); break;
    case A::MEASURE: f.set(3, "Measuring %d/%d s", calib_.measuredS(), (int)(A::MEASURE_MS / 1000)); break;
    default:
      if (calib_.shockInStep()) f.set(3, "Moved! Redo %s up", axis);
      else                      f.set(3, "Turn %s up, hold", axis);
      break;
  }
  f.set(4, "a %+5.1f %+5.1f %+5.1f", d.ax, d.ay, d.az);
  f.set(5, "Detach IMU, lay it");
  f.set(6, "flat, same axes back");
  f.set(7, "%cCancel", blink);
}
