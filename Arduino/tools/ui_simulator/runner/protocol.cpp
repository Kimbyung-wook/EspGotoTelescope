#include "protocol.h"

#include <cstdio>

#include "json_mini.h"

namespace sim {

namespace {

void readGps(const json::Value& v, GpsState& g) {
  g.year = v.getInt("year", g.year);
  g.month = v.getInt("month", g.month);
  g.day = v.getInt("day", g.day);
  g.hour = v.getInt("hour", g.hour);
  g.minute = v.getInt("minute", g.minute);
  g.seconds = v.getInt("seconds", g.seconds);
  g.milliseconds = v.getInt("milliseconds", g.milliseconds);
  g.latitude = v.getNumber("lat", g.latitude);
  g.longitude = v.getNumber("lon", g.longitude);
  g.fixquality = v.getInt("fix", g.fixquality);
  g.satellites = v.getInt("sats", g.satellites);
  g.link = v.getBool("link", g.link);
}

void readImu(const json::Value& v, ImuState& m) {
  m.ax = v.getNumber("ax", m.ax);
  m.ay = v.getNumber("ay", m.ay);
  m.az = v.getNumber("az", m.az);
  m.wx = v.getNumber("wx", m.wx);
  m.wy = v.getNumber("wy", m.wy);
  m.wz = v.getNumber("wz", m.wz);
  m.mx = v.getNumber("mx", m.mx);
  m.my = v.getNumber("my", m.my);
  m.mz = v.getNumber("mz", m.mz);
  m.temperature = v.getNumber("temp", m.temperature);
}

}  // namespace

Session::Session(UiBackend* backend) : backend_(backend) {
  frame_.clear();
  restart();
}

void Session::restart() {
  now_ms_ = 0;
  pending_ = InputSnapshot();
  motor_ = MotorOutput();
  backend_->begin(boot_, sensors_);
}

std::string Session::hello() const {
  char buf[160];
  std::snprintf(buf, sizeof(buf),
                "{\"type\":\"hello\",\"version\":%d,\"backend\":\"%s\","
                "\"lines\":%d,\"cols\":%d,\"frame_ms\":%d}",
                kProtocolVersion, backend_->name(), kLines, kCols, kFrameMs);
  return buf;
}

std::string Session::error(const std::string& msg) const {
  std::string out = "{\"type\":\"error\",\"msg\":";
  json::appendString(out, msg.c_str());
  out += "}";
  return out;
}

std::string Session::step() {
  Frame f;
  f.clear();
  backend_->frame(now_ms_, pending_, sensors_, f, motor_);
  pending_.clicks = 0;
  pending_.phone_time = false;
  frame_ = f;

  char head[96];
  std::snprintf(head, sizeof(head), "{\"type\":\"frame\",\"seq\":%lu,\"t_ms\":%lu,\"lines\":[",
                seq_, now_ms_);
  std::string out = head;
  for (int i = 0; i < kLines; i++) {
    if (i) out += ',';
    json::appendString(out, f.lines[i]);
  }
  char tail[96];
  std::snprintf(tail, sizeof(tail), "],\"motor\":{\"in1\":%d,\"in2\":%d,\"pwm\":%d}}",
                motor_.in1, motor_.in2, motor_.pwm);
  out += tail;

  seq_++;
  now_ms_ += kFrameMs;
  return out;
}

bool Session::handleLine(const std::string& line, std::vector<std::string>& replies) {
  // Ignore blank lines (e.g. a stray "\r").
  bool blank = true;
  for (char c : line) if (c != ' ' && c != '\r' && c != '\t') blank = false;
  if (blank) return true;

  json::Value msg;
  std::string err;
  if (!json::parse(line, msg, err)) { replies.push_back(error("bad json: " + err)); return true; }
  if (!msg.isObject()) { replies.push_back(error("message must be an object")); return true; }

  std::string type = msg.getString("type", "");
  if (type == "input") {
    pending_.count += msg.getInt("rotate", 0);
    pending_.clicks += msg.getInt("click", 0);
  } else if (type == "sensors") {
    if (const json::Value* g = msg.get("gps")) readGps(*g, sensors_.gps);
    if (const json::Value* m = msg.get("imu")) readImu(*m, sensors_.imu);
    if (const json::Value* p = msg.get("power")) sensors_.voltage = p->getNumber("v", sensors_.voltage);
    if (const json::Value* r = msg.get("remote")) sensors_.remote = r->getInt("state", sensors_.remote);
  } else if (type == "phone_time") {
    const double s = msg.getNumber("unix_s", 0.0);
    pending_.phone_time = s > 0.0;
    pending_.phone_unix_s = s > 0.0 ? (unsigned long)s : 0;
    pending_.phone_ms = msg.getInt("ms", 0);
  } else if (type == "tick") {
    int n = msg.getInt("n", 1);
    if (n < 1) n = 1;
    if (n > 1000) n = 1000;
    for (int i = 0; i < n; i++) replies.push_back(step());
  } else if (type == "reset") {
    boot_.imu_ok = msg.getBool("imu_ok", boot_.imu_ok);
    boot_.gps_ok = msg.getBool("gps_ok", boot_.gps_ok);
    restart();
  } else if (type == "hello") {
    replies.push_back(hello());
  } else if (type == "quit") {
    return false;
  } else {
    replies.push_back(error("unknown type: " + type));
  }
  return true;
}

}  // namespace sim
