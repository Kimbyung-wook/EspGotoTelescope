"""Talk to ui_runner.exe over stdin/stdout (one JSON object per line).

See ../PROTOCOL.md. Used by simulator.py (GUI) and protocol_check.py (headless).
"""
import json
import os
import queue
import subprocess
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_RUNNER = os.path.normpath(os.path.join(HERE, "..", "build", "ui_runner.exe"))


class RunnerClient:
    def __init__(self, runner=DEFAULT_RUNNER, args=()):
        if not os.path.exists(runner):
            raise FileNotFoundError(
                "runner not found: %s\nbuild it first: bash build.sh" % runner)
        flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        self.proc = subprocess.Popen(
            [runner, *args], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True, encoding="ascii", bufsize=1,
            creationflags=flags)
        self.messages = queue.Queue()
        self._reader = threading.Thread(target=self._read, daemon=True)
        self._reader.start()
        self.hello = self.wait_for("hello", timeout=5.0)

    def _read(self):
        for line in self.proc.stdout:
            line = line.strip()
            if not line:
                continue
            try:
                self.messages.put(json.loads(line))
            except json.JSONDecodeError:
                self.messages.put({"type": "error", "msg": "bad line: " + line})
        self.messages.put({"type": "eof"})

    def send(self, msg):
        self.proc.stdin.write(json.dumps(msg, separators=(",", ":")) + "\n")
        self.proc.stdin.flush()

    # Convenience wrappers
    def rotate(self, steps):
        self.send({"type": "input", "rotate": int(steps)})

    def click(self, n=1):
        self.send({"type": "input", "click": int(n)})

    def sensors(self, gps=None, imu=None):
        msg = {"type": "sensors"}
        if gps:
            msg["gps"] = gps
        if imu:
            msg["imu"] = imu
        self.send(msg)

    def reset(self, imu_ok=True, gps_ok=True):
        self.send({"type": "reset", "imu_ok": imu_ok, "gps_ok": gps_ok})

    def tick(self, n=1):
        self.send({"type": "tick", "n": int(n)})

    def wait_for(self, mtype, timeout=5.0):
        while True:
            msg = self.messages.get(timeout=timeout)
            if msg.get("type") == "eof":
                raise EOFError("runner exited")
            if msg.get("type") == mtype:
                return msg
            if msg.get("type") == "error":
                raise RuntimeError("runner error: %s" % msg.get("msg"))

    def step(self, n=1, timeout=5.0):
        """Advance n frames synchronously and return the last frame."""
        self.tick(n)
        frame = None
        for _ in range(n):
            frame = self.wait_for("frame", timeout)
        return frame

    def close(self):
        if self.proc.poll() is None:
            try:
                self.send({"type": "quit"})
                self.proc.wait(timeout=2)
            except Exception:
                self.proc.kill()
