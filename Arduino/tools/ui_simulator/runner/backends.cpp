// Backends compiled into the runner. To plug in the real firmware UI, add a
// class implementing sim::UiBackend (e.g. firmware_ui.cpp) and register it here.
#include <cstring>

#include "mock_ui.h"
#include "ui_backend.h"

namespace sim {

UiBackend* createBackend(const char* name) {
  if (std::strcmp(name, "mock") == 0) return new MockUi();
  // if (std::strcmp(name, "firmware") == 0) return new FirmwareUi();
  return nullptr;
}

}  // namespace sim
