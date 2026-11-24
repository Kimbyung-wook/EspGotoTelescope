#pragma once
#ifndef _REMOTE_UI_H_
#define _REMOTE_UI_H_
#include "../board_config.h"
#include "../ui/frame.h"

/************************************/
// Remote UI (stage 0 of docs/REMOTE_UI_PLAN.md)
//   ESP8266 SoftAP + one small HTTP page + WebSocket server.
//   ESP -> phone : {"lines":[8 lines]}         (only when the frame changed)
//   phone -> ESP : {"rotate":n} / {"click":1}   (same keys as tools/ui_simulator/PROTOCOL.md)
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
int  clients();                   // connected WebSocket clients
#else
void begin();                     // Wi-Fi off (the SDK may boot with a saved mode)
inline void poll() {}
inline void publish(const Frame&) {}
inline void takeInput(int& rotate, bool& click) { rotate = 0; click = false; }
inline int  clients() { return 0; }
#endif

} // namespace remote

#endif // _REMOTE_UI_H_
