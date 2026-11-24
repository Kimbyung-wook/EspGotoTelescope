#pragma once
#ifndef _UI_SCREENS_CALIBRATION_H_
#define _UI_SCREENS_CALIBRATION_H_
#include "widgets.h"
#include "../../estimation/calibration.h"

/************************************/
// Calibration (DES-CAL-01..05): the motor is stopped while these are on top.
// Same flow for both: measuring -> Cancel, result -> Apply / Discard,
// failed -> Retry / Quit. Apply writes the result to the settings and saves.
/************************************/
template <class Calib>
class CalibScreen : public Screen {
public:
  void onEnter(UiContext& ctx) override { calib_.start(ctx.tel.now_ms); choice_.reset(); }
  void onRotate(int delta, UiContext& /*ctx*/) override { choice_.rotate(delta); }
  void tick(UiContext& ctx) override {
    ctx.state.motor_inhibit = true;
    calib_.update(ctx.tel.now_ms, ctx.tel.imu.raw);
  }
  void onClick(ScreenManager& mgr, UiContext& ctx) override {
    switch (calib_.state()) {
      case Calib::RESULT:
        if (choice_.first()) {                  // Apply : used from now on and saved
          apply(ctx.state.cfg);
          ctx.state.settings_save_request = true;
        }
        mgr.pop();
        break;
      case Calib::FAILED:
        if (choice_.first()) { calib_.start(ctx.tel.now_ms); choice_.reset(); }   // Retry
        else mgr.pop();
        break;
      default:
        mgr.pop();                              // Cancel : old values stay
        break;
    }
  }
  const Calib& calibration() const { return calib_; }
protected:
  virtual void apply(config::Settings& cfg) const = 0;
  Calib calib_;
  Choice choice_;
};

class GyroCalibScreen : public CalibScreen<cal::GyroCalibration> {
public:
  void render(Frame& f, const UiContext& ctx, char blink) override;
protected:
  void apply(config::Settings& cfg) const override;
};

class AccelCalibScreen : public CalibScreen<cal::AccelCalibration> {
public:
  void render(Frame& f, const UiContext& ctx, char blink) override;
protected:
  void apply(config::Settings& cfg) const override;
};

#endif // _UI_SCREENS_CALIBRATION_H_
