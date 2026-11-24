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
//   Received input is only counted here; merging it with the rotary is stage 2.
//
// With REMOTE_UI_ENABLE == 0 every call is an empty inline function and
// begin() only turns Wi-Fi off, so no network code is linked.
/************************************/
namespace remote {

#if REMOTE_UI_ENABLE
void begin();                     // start AP, HTTP and WebSocket servers
void poll();                      // serve HTTP / WebSocket (call often, e.g. while waiting)
void publish(const Frame& frame); // send the lines if they changed
// Remote input received since the last call (stage 2 adds it to the rotary input)
void takeInput(int& rotate, int& clicks);
int  clients();                   // connected WebSocket clients
#else
void begin();                     // Wi-Fi off (the SDK may boot with a saved mode)
inline void poll() {}
inline void publish(const Frame&) {}
inline void takeInput(int& rotate, int& clicks) { rotate = 0; clicks = 0; }
inline int  clients() { return 0; }
#endif

} // namespace remote

#endif // _REMOTE_UI_H_
