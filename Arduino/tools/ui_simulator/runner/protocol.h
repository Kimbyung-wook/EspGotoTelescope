// Protocol session: turns one-line JSON messages from the GUI into calls on a
// UiBackend, and frames back into one-line JSON replies. See PROTOCOL.md.
// Kept free of stdin/stdout so tests can drive it directly.
#pragma once
#include <string>
#include <vector>

#include "ui_backend.h"

namespace sim {

const int kProtocolVersion = 1;

class Session {
 public:
  explicit Session(UiBackend* backend);

  // The greeting sent once when the runner starts.
  std::string hello() const;

  // Handle one message line. Appends zero or more reply lines (no newline).
  // Returns false when the GUI asked the runner to quit.
  bool handleLine(const std::string& line, std::vector<std::string>& replies);

  // Run one 10 Hz frame and return its JSON line (also used by tests).
  std::string step();

  const Frame& lastFrame() const { return frame_; }
  const MotorOutput& lastMotor() const { return motor_; }
  SensorState& sensors() { return sensors_; }

 private:
  void restart();
  std::string error(const std::string& msg) const;

  UiBackend* backend_;
  BootConfig boot_;
  SensorState sensors_;
  InputSnapshot pending_;     // count accumulates, clicks are consumed per frame
  unsigned long now_ms_ = 0;
  unsigned long seq_ = 0;
  Frame frame_;
  MotorOutput motor_;
};

}  // namespace sim
