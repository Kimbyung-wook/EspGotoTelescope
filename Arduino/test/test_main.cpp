#include <cstring>
#include "test_main.h"
#include "Arduino.h"

int g_failures = 0;
int g_checks = 0;
namespace stub { unsigned long now_ms = 0; unsigned long now_us = 0; int pin_level[32]; int pin_analog[32]; }

static struct { const char* name; TestFn fn; } g_tests[256];
static int g_count = 0;
void register_test(const char* name, TestFn fn) { g_tests[g_count].name = name; g_tests[g_count].fn = fn; g_count++; }

int main() {
  for (int i = 0; i < g_count; i++) {
    int before = g_failures;
    std::memset(stub::pin_level, 0, sizeof(stub::pin_level));
    std::memset(stub::pin_analog, 0, sizeof(stub::pin_analog));
    stub::now_ms = 0; stub::now_us = 0;
    g_tests[i].fn();
    std::printf("%s %s\n", g_failures == before ? "[ OK ]" : "[FAIL]", g_tests[i].name);
  }
  std::printf("\n%d tests, %d checks, %d failures\n", g_count, g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
