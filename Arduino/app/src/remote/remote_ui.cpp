#include "remote_ui.h"
#include <ESP8266WiFi.h>

#if !REMOTE_UI_ENABLE

namespace remote {
// Arduino core 3.x already boots with the modem asleep (__disableWiFiAtBootTime),
// but say it explicitly so a saved SDK mode or a core change cannot turn the AP on.
void begin() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
}
} // namespace remote

#else // REMOTE_UI_ENABLE

#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <stdlib.h>
#include <string.h>

#include "remote_page.h"   // web/index.html (scripts/gen_remote_page.py)

namespace remote {
namespace {

// Stage 1: mirror only. The page shows its input buttons when this is true (stage 2).
constexpr bool INPUT_ENABLED = false;
constexpr unsigned long RESEND_MS = 1000;   // resend an unchanged frame (page "no data" check)

ESP8266WebServer http(REMOTE_UI_HTTP_PORT);
WebSocketsServer ws(REMOTE_UI_WS_PORT);

Frame last;               // last frame sent
bool  force_send = true;  // a new client needs the current frame
unsigned long sent_ms = 0;
// worst case: 8 lines x 21 chars x 6 bytes ("\u00XX") + quotes and commas
char  json[16 + Frame::LINES * ((Frame::COLS - 1) * 6 + 3)];

int pending_rotate = 0, pending_clicks = 0;  // since takeInput()

// JSON string body of one screen line (same escaping as the UI simulator runner)
size_t appendLine(char* out, size_t pos, const char* line) {
  static const char HEX_DIGITS[] = "0123456789abcdef";
  out[pos++] = '"';
  for (int i = 0; i < Frame::COLS - 1 && line[i]; i++) {
    unsigned char ch = (unsigned char)line[i];
    if (ch == '"' || ch == '\\') {
      out[pos++] = '\\';
      out[pos++] = (char)ch;
    } else if (ch < 0x20 || ch >= 0x7F) {
      memcpy(out + pos, "\\u00", 4);
      pos += 4;
      out[pos++] = HEX_DIGITS[ch >> 4];
      out[pos++] = HEX_DIGITS[ch & 0xF];
    } else {
      out[pos++] = (char)ch;
    }
  }
  out[pos++] = '"';
  return pos;
}

// Integer value of "key" in a small flat JSON object, or false if absent
bool findInt(const char* msg, const char* key, long& value) {
  const char* p = strstr(msg, key);
  if (!p) return false;
  p = strchr(p + strlen(key), ':');
  if (!p) return false;
  char* end = nullptr;
  value = strtol(p + 1, &end, 10);
  return end != p + 1;
}

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    ws.sendTXT(num, INPUT_ENABLED ? "{\"hello\":{\"input\":true}}" : "{\"hello\":{\"input\":false}}");
    force_send = true;
    return;
  }
  if (!INPUT_ENABLED) return;
  if (type != WStype_TEXT || length == 0 || length > 64) return;
  char msg[65];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  long v = 0;
  if (findInt(msg, "\"rotate\"", v) && v >= -100 && v <= 100) pending_rotate += (int)v;
  if (findInt(msg, "\"click\"", v) && v > 0) pending_clicks += 1;
}

} // namespace

void begin() {
  WiFi.persistent(false);  // do not write the AP settings to flash
  WiFi.mode(WIFI_AP);
  WiFi.setOutputPower(REMOTE_UI_TX_DBM);
  // one station at a time : only one phone can drive the mount
  WiFi.softAP(REMOTE_UI_AP_SSID, REMOTE_UI_AP_PASS, REMOTE_UI_AP_CHANNEL, 0, 1);

  http.on("/", []() { http.send_P(200, "text/html; charset=utf-8", REMOTE_PAGE); });
  http.onNotFound([]() { http.send(404, "text/plain", "not found"); });
  http.begin();

  ws.begin();
  ws.onEvent(onWsEvent);
}

void poll() {
  http.handleClient();
  ws.loop();
}

void publish(const Frame& frame) {
  if (ws.connectedClients() == 0) {
    force_send = true;
    return;
  }
  const unsigned long now = millis();
  if (!force_send && now - sent_ms < RESEND_MS &&
      memcmp(last.lines, frame.lines, sizeof(last.lines)) == 0) return;
  memcpy(last.lines, frame.lines, sizeof(last.lines));
  force_send = false;
  sent_ms = now;

  size_t pos = 0;
  memcpy(json, "{\"lines\":[", 10);
  pos = 10;
  for (int r = 0; r < Frame::LINES; r++) {
    if (r) json[pos++] = ',';
    pos = appendLine(json, pos, frame.lines[r]);
  }
  json[pos++] = ']';
  json[pos++] = '}';
  json[pos] = '\0';
  ws.broadcastTXT(json, pos);
}

void takeInput(int& rotate, int& clicks) {
  rotate = pending_rotate;
  clicks = pending_clicks;
  pending_rotate = 0;
  pending_clicks = 0;
}

int clients() { return ws.connectedClients(); }

} // namespace remote

#endif // REMOTE_UI_ENABLE
