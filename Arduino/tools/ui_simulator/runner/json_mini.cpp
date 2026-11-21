#include "json_mini.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace json {

const Value* Value::get(const char* key) const {
  if (type != kObject) return nullptr;
  for (const auto& kv : obj)
    if (kv.first == key) return &kv.second;
  return nullptr;
}

double Value::getNumber(const char* key, double def) const {
  const Value* v = get(key);
  if (!v) return def;
  if (v->type == kNumber) return v->num;
  if (v->type == kBool) return v->b ? 1.0 : 0.0;
  return def;
}

int Value::getInt(const char* key, int def) const {
  double d = getNumber(key, (double)def);
  return (int)(d < 0 ? d - 0.5 : d + 0.5);
}

bool Value::getBool(const char* key, bool def) const {
  const Value* v = get(key);
  if (!v) return def;
  if (v->type == kBool) return v->b;
  if (v->type == kNumber) return v->num != 0.0;
  return def;
}

std::string Value::getString(const char* key, const char* def) const {
  const Value* v = get(key);
  if (!v || v->type != kString) return def;
  return v->str;
}

namespace {

struct Parser {
  const char* p;
  const char* end;
  std::string err;

  void ws() { while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++; }
  bool fail(const char* msg) { if (err.empty()) err = msg; return false; }

  bool literal(const char* word) {
    size_t n = std::strlen(word);
    if ((size_t)(end - p) < n || std::strncmp(p, word, n) != 0) return fail("bad literal");
    p += n;
    return true;
  }

  bool hex4(unsigned& code) {
    if (end - p < 4) return fail("short \\u escape");
    code = 0;
    for (int i = 0; i < 4; i++) {
      char c = *p++;
      code <<= 4;
      if (c >= '0' && c <= '9') code |= (unsigned)(c - '0');
      else if (c >= 'a' && c <= 'f') code |= (unsigned)(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F') code |= (unsigned)(c - 'A' + 10);
      else return fail("bad \\u escape");
    }
    return true;
  }

  bool string(std::string& out) {
    if (p >= end || *p != '"') return fail("expected string");
    p++;
    while (p < end && *p != '"') {
      char c = *p++;
      if (c != '\\') { out += c; continue; }
      if (p >= end) return fail("bad escape");
      char e = *p++;
      switch (e) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          unsigned code;
          if (!hex4(code)) return false;
          out += (char)(code <= 0xFF ? code : '?');   // display is 8-bit
          break;
        }
        default: return fail("bad escape");
      }
    }
    if (p >= end) return fail("unterminated string");
    p++;
    return true;
  }

  bool value(Value& v, int depth) {
    if (depth > 32) return fail("too deep");
    ws();
    if (p >= end) return fail("unexpected end");
    char c = *p;
    if (c == '{') {
      v.type = Value::kObject;
      p++;
      ws();
      if (p < end && *p == '}') { p++; return true; }
      for (;;) {
        ws();
        std::string key;
        if (!string(key)) return false;
        ws();
        if (p >= end || *p != ':') return fail("expected ':'");
        p++;
        Value child;
        if (!value(child, depth + 1)) return false;
        v.obj.emplace_back(key, child);
        ws();
        if (p < end && *p == ',') { p++; continue; }
        if (p < end && *p == '}') { p++; return true; }
        return fail("expected ',' or '}'");
      }
    }
    if (c == '[') {
      v.type = Value::kArray;
      p++;
      ws();
      if (p < end && *p == ']') { p++; return true; }
      for (;;) {
        Value child;
        if (!value(child, depth + 1)) return false;
        v.arr.push_back(child);
        ws();
        if (p < end && *p == ',') { p++; continue; }
        if (p < end && *p == ']') { p++; return true; }
        return fail("expected ',' or ']'");
      }
    }
    if (c == '"') { v.type = Value::kString; return string(v.str); }
    if (c == 't') { v.type = Value::kBool; v.b = true; return literal("true"); }
    if (c == 'f') { v.type = Value::kBool; v.b = false; return literal("false"); }
    if (c == 'n') { v.type = Value::kNull; return literal("null"); }
    if (c == '-' || (c >= '0' && c <= '9')) {
      std::string num;
      while (p < end && (std::strchr("+-0123456789.eE", *p) != nullptr)) num += *p++;
      char* stop = nullptr;
      v.type = Value::kNumber;
      v.num = std::strtod(num.c_str(), &stop);
      if (stop == num.c_str() || *stop != '\0') return fail("bad number");
      return true;
    }
    return fail("unexpected character");
  }
};

}  // namespace

bool parse(const std::string& text, Value& out, std::string& error) {
  Parser ps{text.data(), text.data() + text.size(), std::string()};
  out = Value();
  if (!ps.value(out, 0)) { error = ps.err; return false; }
  ps.ws();
  if (ps.p != ps.end) { error = "trailing characters"; return false; }
  return true;
}

void appendString(std::string& out, const char* s) {
  out += '"';
  for (const unsigned char* q = (const unsigned char*)s; *q; q++) {
    unsigned char c = *q;
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c < 0x20 || c >= 0x7F) {
      char buf[8];
      std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
      out += buf;
    } else {
      out += (char)c;
    }
  }
  out += '"';
}

}  // namespace json
