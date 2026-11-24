"""Headless check of ui_runner.exe over the real stdin/stdout protocol.

    python protocol_check.py [--runner PATH] [-v]

Starts the runner, boots it, enters and leaves every main menu screen and
checks the title line, the RA speed edit + motor output, sensor updates,
error replies, and that every frame line renders within 128x64.
Exits non-zero on the first failure. No tkinter needed.
"""
import argparse
import sys

import oled_render
from runner_client import DEFAULT_RUNNER, RunnerClient

TITLES = ["*1Target", "*2Polar Align", "*3Status", "*4Calibration", "*5Settings",
          "*6RA Speed", " "]   # Sleep blanks the title line
# rotations from the first row to Back/Quit; None = click leaves
EXIT_ROTATE = [5, 1, 6, 2, 7, 2, None]

failures = 0
verbose = False


def check(cond, what, frame=None):
    global failures
    if cond:
        if verbose:
            print("ok   " + what)
        return
    failures += 1
    print("FAIL " + what)
    if frame:
        print("     " + "\n     ".join(frame["lines"]))


def check_frame_shape(f):
    check(len(f["lines"]) == 8, "frame has 8 lines")
    check(all(len(l) <= 21 for l in f["lines"]), "lines are at most 21 chars", f)
    px = oled_render.render_lines(f["lines"])
    check(len(px) == 64 and len(px[0]) == 128, "rendered 128x64")


def main():
    global verbose
    ap = argparse.ArgumentParser()
    ap.add_argument("--runner", default=DEFAULT_RUNNER)
    ap.add_argument("-v", action="store_true", help="print passing checks too")
    args = ap.parse_args()
    verbose = args.v

    c = RunnerClient(args.runner)
    try:
        check(c.hello["version"] == 1 and c.hello["lines"] == 8 and c.hello["cols"] == 21,
              "hello: version 1, 8x21")
        check(c.hello["backend"] == "firmware", "backend is the firmware UI")

        f = c.step()
        check(f["lines"][0] == "SimpleGoTo v0.3.1", "boot starts with the name and version", f)
        for _ in range(100):
            f = c.step()
            if f["lines"][1] == "Main Menu":
                break
        check(f["lines"][1] == "Main Menu", "main menu after boot", f)
        check_frame_shape(f)
        check(f["lines"][2][1:] == "1Target" and f["lines"][2][0] in ">-",
              "cursor on first item", f)

        for k, title in enumerate(TITLES):
            c.rotate(k)
            f = c.step()
            c.click()
            f = c.step()
            check(f["lines"][1] == title, "enter %s" % title, f)
            check_frame_shape(f)
            if EXIT_ROTATE[k] is not None:
                c.rotate(EXIT_ROTATE[k])
                c.step()
            c.click()
            f = c.step()
            check(f["lines"][1] == "Main Menu", "leave %s" % title, f)
            c.rotate(-20)   # back to the top for the next item
            c.step()

        # RA speed edit drives the motor
        c.rotate(5); c.step()
        c.click(); c.step()
        c.click(); c.step()
        c.rotate(-30)
        f = c.step()
        check(f["lines"][1] == "*Set RA Speed", "RA edit mode", f)
        check(f["motor"] == {"in1": 1, "in2": 0, "pwm": 30}, "motor reverse pwm 30 (%s)" % f["motor"])
        c.click(); c.step()
        c.rotate(2); c.step()
        c.click()
        f = c.step()
        check(f["lines"][1] == "Main Menu", "leave RA speed", f)
        c.rotate(-20); c.step()

        # sensor values reach the screen
        c.sensors(gps={"hour": 23, "minute": 59, "seconds": 58, "milliseconds": 900},
                  imu={"wz": -12.5})
        c.rotate(2); c.step()
        c.click(); c.step()                  # 3Status
        c.rotate(1); c.step()
        c.click()
        f = c.step()                         # Acc/Gyro
        check(f["lines"][0] == "T 23:59:58.9 V 12.60", "status bar shows GPS time", f)
        check(f["lines"][7] == "Wz     -12.50000 dps", "Wz on Acc/Gyro screen", f)
        c.click(); c.step()

        # reset with failing devices
        c.reset(imu_ok=False, gps_ok=False)
        f = c.step(25)
        check(f["lines"][1] == ">> IMU failed" and f["lines"][5] == ">> GPS is timeout",
              "boot failure lines", f)

        # errors
        c.send({"type": "nope"})
        try:
            c.wait_for("frame", timeout=2)
            check(False, "unknown type -> error")
        except RuntimeError as e:
            check("unknown type" in str(e), "unknown type -> error")
        c.proc.stdin.write("{broken\n")
        c.proc.stdin.flush()
        try:
            c.wait_for("frame", timeout=2)
            check(False, "bad json -> error")
        except RuntimeError as e:
            check("bad json" in str(e), "bad json -> error")
    finally:
        c.close()
    check(c.proc.returncode == 0, "runner exits cleanly on quit")

    print("protocol check: %s" % ("OK" if failures == 0 else "%d failure(s)" % failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
