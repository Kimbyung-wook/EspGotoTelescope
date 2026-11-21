// Backends compiled into the runner. To add another UI, implement
// sim::UiBackend and register it here.
#include <cstring>

#include "firmware_ui.h"
#include "ui_backend.h"

namespace sim {

UiBackend* createBackend(const char* name) {
  if (std::strcmp(name, "firmware") == 0) return new FirmwareUi();
  return nullptr;
}

}  // namespace sim
