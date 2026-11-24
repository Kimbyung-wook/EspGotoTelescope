"""Phone remote UI on the PC: the firmware UI runner behind the same web page the ESP8266 serves.

    python gui/web_bridge.py                -> open http://localhost:8080/  (WebSocket on 8081)
    python gui/web_bridge.py --mirror-only  -> page input ignored, as in the stage 1 firmware

Serves app/src/remote/web/index.html over HTTP, runs ../build/ui_runner.exe at 10 Hz and
talks to the page with the remote UI messages (docs/REMOTE_UI_PLAN.md):
    bridge -> page : {"hello":{"input":true}}, {"lines":[8 lines]}  (on change, at least every 1 s)
    page -> bridge : {"rotate":n}, {"click":1}                       (forwarded as runner "input")
Standard library only (a minimal RFC 6455 text-frame WebSocket server).
"""
import argparse
import base64
import hashlib
import http.server
import json
import os
import socketserver
import struct
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
SIM = os.path.dirname(HERE)
ARDUINO = os.path.dirname(os.path.dirname(SIM))
PAGE = os.path.join(ARDUINO, "app", "src", "remote", "web", "index.html")
DEFAULT_RUNNER = os.path.join(SIM, "build", "ui_runner.exe" if os.name == "nt" else "ui_runner")
WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


class Runner:
    """ui_runner.exe over stdin/stdout (tools/ui_simulator/PROTOCOL.md)."""

    def __init__(self, path):
        self.p = subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1)
        self.lock = threading.Lock()
        self._read("hello")

    def _send(self, obj):
        self.p.stdin.write(json.dumps(obj) + "\n")
        self.p.stdin.flush()

    def _read(self, want):
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise RuntimeError("runner exited")
            msg = json.loads(line)
            if msg.get("type") == want:
                return msg

    def input(self, rotate=0, click=0):
        with self.lock:
            self._send({"type": "input", "rotate": rotate, "click": click})

    def tick(self):
        with self.lock:
            self._send({"type": "tick"})
            return self._read("frame")

    def close(self):
        try:
            self._send({"type": "quit"})
        except OSError:
            pass


class Clients:
    def __init__(self, input_enabled=True):
        self.input_enabled = input_enabled
        self.lock = threading.Lock()
        self.socks = set()
        self.last = None

    def add(self, s):
        with self.lock:
            self.socks.add(s)
            last = self.last
        send_text(s, json.dumps({"hello": {"input": self.input_enabled}}))
        if last:
            send_text(s, last)

    def remove(self, s):
        with self.lock:
            self.socks.discard(s)

    def broadcast(self, text):
        with self.lock:
            self.last = text
            socks = list(self.socks)
        for s in socks:
            try:
                send_text(s, text)
            except OSError:
                self.remove(s)


def send_text(sock, text):
    data = text.encode()
    n = len(data)
    if n < 126:
        head = struct.pack("!BB", 0x81, n)
    elif n < 65536:
        head = struct.pack("!BBH", 0x81, 126, n)
    else:
        head = struct.pack("!BBQ", 0x81, 127, n)
    sock.sendall(head + data)


def recv_exact(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError
        buf += chunk
    return buf


def recv_frame(sock):
    b1, b2 = recv_exact(sock, 2)
    op, n = b1 & 0x0F, b2 & 0x7F
    if n == 126:
        n = struct.unpack("!H", recv_exact(sock, 2))[0]
    elif n == 127:
        n = struct.unpack("!Q", recv_exact(sock, 8))[0]
    mask = recv_exact(sock, 4) if b2 & 0x80 else b"\0\0\0\0"
    data = bytes(c ^ mask[i % 4] for i, c in enumerate(recv_exact(sock, n)))
    return op, data


def make_ws_handler(clients, runner):
    class WsHandler(socketserver.BaseRequestHandler):
        def handle(self):
            s = self.request
            req = b""
            while b"\r\n\r\n" not in req:
                chunk = s.recv(1024)
                if not chunk:
                    return
                req += chunk
            key = None
            for line in req.decode(errors="replace").split("\r\n"):
                if line.lower().startswith("sec-websocket-key:"):
                    key = line.split(":", 1)[1].strip()
            if not key:
                return
            accept = base64.b64encode(hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
            s.sendall(("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
                       "Connection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n" % accept).encode())
            clients.add(s)
            try:
                while True:
                    op, data = recv_frame(s)
                    if op == 0x8:          # close
                        break
                    if op == 0x9:          # ping -> pong
                        s.sendall(struct.pack("!BB", 0x8A, len(data)) + data)
                        continue
                    if op != 0x1 or len(data) > 64 or not clients.input_enabled:
                        continue
                    try:
                        msg = json.loads(data)
                    except ValueError:
                        continue
                    r = msg.get("rotate", 0)
                    c = 1 if msg.get("click") else 0
                    if isinstance(r, int) and -100 <= r <= 100 and (r or c):
                        runner.input(r, c)
            except (ConnectionError, OSError):
                pass
            finally:
                clients.remove(s)
    return WsHandler


class PageHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path.split("?")[0] not in ("/", "/index.html"):
            self.send_error(404)
            return
        with open(PAGE, "rb") as f:
            body = f.read()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass


class ThreadingTCP(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", type=int, default=8080, help="HTTP port (WebSocket = port + 1)")
    ap.add_argument("--runner", default=DEFAULT_RUNNER)
    ap.add_argument("--mirror-only", action="store_true",
                    help="ignore page input, like the stage 1 firmware (buttons hidden)")
    args = ap.parse_args()
    if not os.path.exists(args.runner):
        sys.exit("runner not found: %s (run build.sh first)" % args.runner)

    runner = Runner(args.runner)
    clients = Clients(input_enabled=not args.mirror_only)
    http_srv = ThreadingTCP(("", args.port), PageHandler)
    ws_srv = ThreadingTCP(("", args.port + 1), make_ws_handler(clients, runner))
    for srv in (http_srv, ws_srv):
        threading.Thread(target=srv.serve_forever, daemon=True).start()
    print("remote UI page: http://localhost:%d/  (WebSocket %d)" % (args.port, args.port + 1), flush=True)

    prev, sent_at = None, 0.0
    next_t = time.monotonic()
    try:
        while True:
            frame = runner.tick()
            now = time.monotonic()
            if frame["lines"] != prev or now - sent_at >= 1.0:   # same rule as the firmware
                clients.broadcast(json.dumps({"lines": frame["lines"]}))
                prev, sent_at = frame["lines"], now
            next_t += 0.1
            time.sleep(max(0.0, next_t - time.monotonic()))
    except KeyboardInterrupt:
        pass
    finally:
        runner.close()


if __name__ == "__main__":
    main()
