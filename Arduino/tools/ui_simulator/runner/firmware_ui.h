// UiBackend that runs the real firmware UI (Arduino/app/src/ui, TelescopeUi).
//
// It does on the PC what app.ino does on the board:
//   setup(): the boot log lines (setup() itself is not host-buildable)
//   loop():  rotary delta/click + updateTelemetry() -> AttitudeEstimator
//            -> TelescopeUi::step() -> RaMotor output from UiState::ra_speed_cmd
// Firmware sources are only read, never changed; see build.sh for the list.
#pragma once
#include <memory>

#include "ui_backend.h"
#include "board_config.h"   // Arduino/app/src (only #defines)

class TelescopeUi;         // Arduino/app/src/ui/telescope_ui.h
class AttitudeEstimator;   // Arduino/app/src/estimation/attitude_estimator.h
namespace astro { class TargetTracker; }   // Arduino/app/src/astro/target_tracker.h
struct UiContext;    // Arduino/app/src/ui/ui_context.h

namespace sim {

class FirmwareUi : public UiBackend {
 public:
  // Boot timing in frames (10 Hz): one boot line every kBootLineFrames
  // (frame 0 shows line 1), the full log is held for kBootHoldFrames frames,
  // and frame 7*kBootLineFrames + kBootHoldFrames is the first UI frame.
  static const int kBootLineFrames = 3;
  static const int kBootHoldFrames = 10;
  static const int kBootFrames = 7 * kBootLineFrames + kBootHoldFrames;
  // ATT_EST_TAU_S in app.ino (not includable from the host build)
  static constexpr float kAttEstTauS = 1.0f;
  // RA_DUTY_AT_SIDEREAL of board_config.h, taken from the firmware header itself
  static constexpr float kRaDutyAtSidereal = RA_DUTY_AT_SIDEREAL;

  FirmwareUi();
  ~FirmwareUi() override;

  const char* name() const override { return "firmware"; }
  void begin(const BootConfig& boot, const SensorState& sensors) override;
  void frame(unsigned long now_ms, const InputSnapshot& input,
             const SensorState& sensors, Frame& out,
             MotorOutput& motor) override;

  // For tests
  bool booting() const { return boot_frame_ < kBootFrames; }
  TelescopeUi& ui() { return *ui_; }
  UiContext& context() { return *ctx_; }

 private:
  void renderBoot(Frame& out);

  std::unique_ptr<TelescopeUi> ui_;
  std::unique_ptr<UiContext> ctx_;
  std::unique_ptr<AttitudeEstimator> att_est_;
  std::unique_ptr<astro::TargetTracker> moon_tracker_;
  std::unique_ptr<astro::TargetTracker> target_tracker_;
  BootConfig boot_;
  int boot_frame_ = 0;
  int last_count_ = 0;
};

}  // namespace sim
