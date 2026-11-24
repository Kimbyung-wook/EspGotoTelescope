#pragma once
#ifndef _REMOTE_UI_H_
#define _REMOTE_UI_H_
#include "../board_config.h"
#include "../ui/frame.h"

/************************************/
// Remote UI (docs/REMOTE_UI_PLAN.md)
//   ESP8266 SoftAP + one small HTTP page + WebSocket server.
//   ESP -> phone : {"hello":{"input":true,"name":..,"version":..}} on connect,
//                  {"lines":[8 lines]}         (when the frame changed, at least every 1 s)
//   phone -> ESP : {"rotate":n} / {"click":1}   (same keys as tools/ui_simulator/PROTOCOL.md)
//                  {"time_s":unix,"time_ms":ms} once after the hello (phone clock, DES-LOC-05)
//   Stage 2: loop() adds the remote input to the rotary input every frame.
//
// With REMOTE_UI_ENABLE == 0 every call is an empty inline function and
// begin() only turns Wi-Fi off, so no network code is linked.
/************************************/
namespace remote {

#if REMOTE_UI_ENABLE
void begin();                     // start AP, HTTP and WebSocket servers
void poll();                      // serve HTTP / WebSocket (call often, e.g. while waiting)
void publish(const Frame& frame); // send the lines if they changed
// Remote input since the last call: all rotate steps and at most one click (the UI
// takes one click per frame; further clicks wait for the next frames).
void takeInput(int& rotate, bool& click);
// Phone clock received since the last call (Unix seconds, milliseconds)
bool takePhoneTime(unsigned long& unix_s, int& ms);
int  clients();                   // connected WebSocket clients
int  state();                     // Telemetry::RemoteState : WAITING (AP on) or CONNECTED
#else
void begin();                     // Wi-Fi off (the SDK may boot with a saved mode)
inline void poll() {}
inline void publish(const Frame&) {}
inline void takeInput(int& rotate, bool& click) { rotate = 0; click = false; }
inline bool takePhoneTime(unsigned long&, int&) { return false; }
inline int  clients() { return 0; }
inline int  state() { return 0; }  // Telemetry::REMOTE_OFF
#endif

} // namespace remote

#endif // _REMOTE_UI_H_
