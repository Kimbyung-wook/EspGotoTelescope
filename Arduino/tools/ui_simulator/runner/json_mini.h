// Very small JSON reader/writer, enough for the one-line protocol messages.
// Supports objects, arrays, strings (with \uXXXX up to 0xFF kept as one byte),
// numbers, true/false/null. No external dependencies (MinGW g++ 6.3, C++14).
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace json {

struct Value {
  enum Type { kNull, kBool, kNumber, kString, kObject, kArray };
  Type type = kNull;
  bool b = false;
  double num = 0.0;
  std::string str;
  std::vector<std::pair<std::string, Value>> obj;
  std::vector<Value> arr;

  bool isObject() const { return type == kObject; }
  // Object member lookup; nullptr if missing or not an object.
  const Value* get(const char* key) const;
  // Typed getters with defaults (numbers also accept bools).
  double getNumber(const char* key, double def) const;
  int getInt(const char* key, int def) const;
  bool getBool(const char* key, bool def) const;
  std::string getString(const char* key, const char* def) const;
};

// Parse one JSON text. Returns false and fills `error` on failure.
bool parse(const std::string& text, Value& out, std::string& error);

// Append `s` as a JSON string literal (quotes included). Bytes >= 0x80 and
// control characters are written as \u00XX, so the 8-bit display character
// codes survive any text encoding on the pipe.
void appendString(std::string& out, const char* s);

}  // namespace json
