// Mock UI backend: a host-only stand-in for the firmware menu, written from
// docs/UI.md. It will be replaced by the real firmware UI (Screen/ScreenManager)
// behind the same UiBackend interface, so keep all mock logic in this class.
#pragma once
#include "ui_backend.h"

namespace sim {

// Cursor over a list of `count` items shown in a window of `rows` lines.
// Moving past the last visible row scrolls the window by one line.
struct ListCursor {
  int count = 1;
  int rows = 6;
  int sel = 0;    // selected item
  int first = 0;  // first visible item

  void reset(int n, int visible = 6) { count = n; rows = visible; sel = 0; first = 0; }
  void move(int delta);
  bool visible(int item) const { return item >= first && item < first + rows; }
};

class MockUi : public UiBackend {
 public:
  enum Screen {
    kBoot,
    kMain,
    kRaSpeed,
    kRaSpeedEdit,
    kStatusGps,
    kStatusAccGyro,
    kStatusMagAlt,
    kStatusImuCalib,
    kDoImuCalib,
    kAttEstimator,
    kHealth,
    kMoonAxis,
    kSleep,
    kCalibAccBias,
    kCalibAccScale,
    kCalibGyroBias,
    kCalibGyroScale,
  };

  // Boot timing in frames (10 Hz): one boot line every kBootLineFrames
  // (frame 0 shows line 1), the full log is held for kBootHoldFrames frames,
  // and frame 7*kBootLineFrames + kBootHoldFrames is the first main menu frame.
  static const int kBootLineFrames = 3;
  static const int kBootHoldFrames = 10;
  static const int kMainItems = 10;
  static const int kAttItems = 13;    // Roll..K_Y + Exit
  static const int kCalibItems = 5;   // 4 calibrations + Quit (blank row skipped)

  const char* name() const override { return "mock"; }
  void begin(const BootConfig& boot, const SensorState& sensors) override;
  void frame(unsigned long now_ms, const InputSnapshot& input,
             const SensorState& sensors, Frame& out,
             MotorOutput& motor) override;

  // For tests
  Screen screen() const { return screen_; }
  double raCommand() const { return ra_cmd_; }

 private:
  void handleRotation(int delta);
  void handleClick(unsigned long now_ms);
  void render(unsigned long now_ms, const SensorState& s, Frame& out);
  void renderBoot(Frame& out);
  void renderStatusBar(const SensorState& s, Frame& out);
  char cur(bool selected) const { return selected ? blink_ : ' '; }
  void updateMotor();
  void updateGyroBias(unsigned long now_ms, const SensorState& s);

  BootConfig boot_;
  Screen screen_ = kBoot;
  int boot_frame_ = 0;
  int last_count_ = 0;
  bool have_count_ = false;
  char blink_ = '>';

  ListCursor main_, ra_, calib_, att_;

  double ra_cmd_ = 0.0;   // RRcmd, also the PWM duty (signed)
  MotorOutput motor_;

  unsigned long calib_start_ms_ = 0;
  double bwx_ = 0, bwy_ = 0, bwz_ = 0;   // gyro bias [dps]
  double bax_ = 0, bay_ = 0, baz_ = 0;   // acc bias (not calibrated by the mock)
};

}  // namespace sim
