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

#define REMOTE_STR2(x) #x
#define REMOTE_STR(x)  REMOTE_STR2(x)

namespace remote {
namespace {

// Stage 0 page : the 8 screen lines and a few input buttons (no dial yet).
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Telescope</title>
<style>body{background:#000;color:#d00;font-family:monospace;margin:12px}
pre{font-size:5vw;line-height:1.15;border:1px solid #400;padding:6px;margin:0 0 12px}
button{background:#200;color:#f22;border:1px solid #600;font-size:20px;padding:12px 0;width:18%}
#st,#rx{font-size:14px;margin-top:8px}</style></head><body>
<pre id="scr">connecting...</pre>
<div><button onclick="s({rotate:-10})">-10</button> <button onclick="s({rotate:-1})">-1</button>
<button onclick="s({click:1})">OK</button> <button onclick="s({rotate:1})">+1</button>
<button onclick="s({rotate:10})">+10</button></div>
<div id="st">-</div><div id="rx"></div>
<script>var ws;function c(){ws=new WebSocket('ws://'+location.hostname+':)HTML" REMOTE_STR(REMOTE_UI_WS_PORT) R"HTML(/');
ws.onopen=function(){st.textContent='connected'};
ws.onclose=function(){st.textContent='disconnected';setTimeout(c,1000)};
ws.onmessage=function(e){var m=JSON.parse(e.data);if(m.lines)scr.textContent=m.lines.join('\n');
if(m.rx)rx.textContent='ESP received: rotate '+m.rx.rotate+', click '+m.rx.click}}
function s(o){if(ws&&ws.readyState==1)ws.send(JSON.stringify(o))}c();</script></body></html>)HTML";

ESP8266WebServer http(REMOTE_UI_HTTP_PORT);
WebSocketsServer ws(REMOTE_UI_WS_PORT);

Frame last;               // last frame sent
bool  force_send = true;  // a new client needs the current frame
// worst case: 8 lines x 21 chars x 6 bytes ("\u00XX") + quotes and commas
char  json[16 + Frame::LINES * ((Frame::COLS - 1) * 6 + 3)];

int  pending_rotate = 0, pending_clicks = 0;  // since takeInput()
long total_rotate   = 0, total_clicks   = 0;  // since boot (echoed back to the phone)

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
    force_send = true;
    return;
  }
  if (type != WStype_TEXT || length == 0 || length > 64) return;
  char msg[65];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  long v = 0;
  bool got = false;
  if (findInt(msg, "\"rotate\"", v) && v >= -100 && v <= 100) {
    pending_rotate += (int)v;
    total_rotate   += v;
    got = true;
  }
  if (findInt(msg, "\"click\"", v) && v > 0) {
    pending_clicks += 1;
    total_clicks   += 1;
    got = true;
  }
  if (got) {
    char ack[64];
    int n = snprintf(ack, sizeof(ack), "{\"rx\":{\"rotate\":%ld,\"click\":%ld}}",
                     total_rotate, total_clicks);
    ws.sendTXT(num, ack, n);
  }
}

} // namespace

void begin() {
  WiFi.persistent(false);  // do not write the AP settings to flash
  WiFi.mode(WIFI_AP);
  WiFi.setOutputPower(REMOTE_UI_TX_DBM);
  // one station at a time : only one phone can drive the mount
  WiFi.softAP(REMOTE_UI_AP_SSID, REMOTE_UI_AP_PASS, REMOTE_UI_AP_CHANNEL, 0, 1);

  http.on("/", []() { http.send_P(200, "text/html", PAGE); });
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
  if (!force_send && memcmp(last.lines, frame.lines, sizeof(last.lines)) == 0) return;
  memcpy(last.lines, frame.lines, sizeof(last.lines));
  force_send = false;

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
