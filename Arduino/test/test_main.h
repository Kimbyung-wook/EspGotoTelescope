// Tiny test helpers for host unit tests.
#pragma once
#include <cstdio>
#include <cmath>
#include <cstring>

extern int g_failures;
extern int g_checks;
#define CHECK(cond) do { g_checks++; if(!(cond)) { g_failures++; \
  std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while(0)
#define CHECK_NEAR(a, b, tol) do { g_checks++; double _a=(a), _b=(b); \
  if(std::fabs(_a-_b) > (tol)) { g_failures++; \
  std::printf("  FAIL %s:%d  %s = %f, expected %f\n", __FILE__, __LINE__, #a, _a, _b); } } while(0)
#define CHECK_STR(a, b) do { g_checks++; if(std::strcmp((a),(b)) != 0) { g_failures++; \
  std::printf("  FAIL %s:%d  \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); } } while(0)

typedef void (*TestFn)();
void register_test(const char* name, TestFn fn);
struct TestRegistrar { TestRegistrar(const char* n, TestFn f) { register_test(n, f); } };
#define TEST(name) static void name(); static TestRegistrar reg_##name(#name, name); static void name()
