"""Telescope OLED UI simulator (tkinter, Python 3.12, no extra packages).

    python simulator.py [--runner PATH] [--backend firmware] [--scale 5] [--shots DIR]

Starts ../build/ui_runner.exe, ticks it at 10 Hz and shows the 128x64 SSD1306
screen it returns, with fake sensor inputs and the RA motor output.
--shots DIR runs a scripted tour through every menu, saves PNG screenshots and
exits (used to verify the GUI without a person at the keyboard).
"""
import argparse
import datetime
import math
import os
import random
import subprocess
import sys
import time
import tkinter as tk
from tkinter import ttk, filedialog

import oled_render
from runner_client import DEFAULT_RUNNER, RunnerClient

G = 9.80665
FRAME_MS = 100

COLOR_SCHEMES = {
    # name: (top 16 rows, rest)
    "흰색": ("#EEF4FF", "#EEF4FF"),
    "파랑": ("#7FD8FF", "#7FD8FF"),
    "노랑/파랑 (2색 패널)": ("#FFD84A", "#7FD8FF"),
}
OFF_COLOR = "#06080C"
GRID_COLOR = "#000000"

# Sensor fields: (group, key, label, default)
GPS_FIELDS = [
    ("year", "년(2자리)", 26), ("month", "월", 11), ("day", "일", 21),
    ("hour", "시", 12), ("minute", "분", 0), ("seconds", "초", 0),
    ("milliseconds", "ms", 0),
    ("lat", "위도 [deg, +N]", 37.5665), ("lon", "경도 [deg, +E]", 126.9780),
    ("fix", "Fix (0 NoFix/1 GPS/2 DGPS)", 1), ("sats", "위성 수", 8),
]
IMU_FIELDS = [
    ("ax", "ax [m/s²]", 0.0), ("ay", "ay [m/s²]", 0.0), ("az", "az [m/s²]", G),
    ("wx", "wx [dps]", 0.0), ("wy", "wy [dps]", 0.0), ("wz", "wz [dps]", 0.0),
    ("mx", "mx [uT]", 20.0), ("my", "my [uT]", 0.0), ("mz", "mz [uT]", -40.0),
    ("temp", "온도 [°C]", 22.0),
]
INT_KEYS = {"year", "month", "day", "hour", "minute", "seconds", "milliseconds", "fix", "sats"}


class Simulator:
    def __init__(self, root, runner_path, scale, backend="firmware"):
        self.root = root
        self.scale = scale
        self.client = RunnerClient(runner_path, ["--backend", backend])
        self.last_frame = None
        self.paused = tk.BooleanVar(value=False)
        self.scheme = tk.StringVar(value="노랑/파랑 (2색 패널)")
        self.grid_on = tk.BooleanVar(value=True)
        self.imu_ok = tk.BooleanVar(value=True)
        self.gps_ok = tk.BooleanVar(value=True)
        self.gps_auto = tk.BooleanVar(value=True)
        self.noise = tk.BooleanVar(value=False)
        self.spin_mag = tk.BooleanVar(value=False)
        self.vars = {}
        self.wheel_acc = 0

        root.title("Telescope OLED UI Simulator  [backend: %s]" % self.client.hello["backend"])
        root.protocol("WM_DELETE_WINDOW", self.quit)
        self._build()
        self._bind_keys()
        self._next_tick = time.perf_counter()
        self.root.after(0, self._tick_loop)
        self.root.after(10, self._poll)

    # ------------------------------------------------------------------ UI
    def _build(self):
        style = ttk.Style()
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass
        self.root.configure(bg=style.lookup("TFrame", "background"))
        left = ttk.Frame(self.root, padding=10)
        left.grid(row=0, column=0, sticky="n")
        right = ttk.Frame(self.root, padding=(0, 10, 10, 10))
        right.grid(row=0, column=1, sticky="n")

        # OLED in a bezel
        bezel = tk.Frame(left, bg="#101215", padx=14, pady=14, highlightthickness=1,
                         highlightbackground="#3A3F47")
        bezel.grid(row=0, column=0)
        w, h = oled_render.WIDTH * self.scale, oled_render.HEIGHT * self.scale
        self.canvas = tk.Canvas(bezel, width=w, height=h, bg=OFF_COLOR,
                                highlightthickness=0, cursor="hand2")
        self.canvas.pack()
        self.base_img = tk.PhotoImage(width=oled_render.WIDTH, height=oled_render.HEIGHT)
        self.zoom_img = self.base_img.zoom(self.scale)
        self.img_item = self.canvas.create_image(0, 0, anchor="nw", image=self.zoom_img)
        self.grid_items = []
        self._draw_grid()
        self.canvas.bind("<MouseWheel>", self._on_wheel)
        self.canvas.bind("<Button-1>", self._on_canvas_click)

        # Rotary controls
        rot = ttk.LabelFrame(left, text="로터리 엔코더", padding=8)
        rot.grid(row=1, column=0, sticky="ew", pady=(10, 0))
        ttk.Button(rot, text="◀ CCW  (← ↑)", command=lambda: self.rotate(-1)).grid(row=0, column=0, padx=4)
        ttk.Button(rot, text="● 클릭  (Enter/Space)", command=self.click).grid(row=0, column=1, padx=4)
        ttk.Button(rot, text="CW ▶  (→ ↓)", command=lambda: self.rotate(+1)).grid(row=0, column=2, padx=4)
        ttk.Label(rot, text="화면 위에서 마우스 휠 = 회전, 화면 클릭 = 버튼").grid(
            row=1, column=0, columnspan=3, pady=(6, 0))

        # Motor output
        mot = ttk.LabelFrame(left, text="RA 모터 출력 (H-브리지)", padding=8)
        mot.grid(row=2, column=0, sticky="ew", pady=(10, 0))
        self.lamp = tk.Canvas(mot, width=150, height=26, highlightthickness=0, bg="#2A2F36")
        self.lamp.grid(row=0, column=0, rowspan=2, padx=(0, 10))
        self.lamp_in1 = self.lamp.create_oval(6, 5, 22, 21, fill="#333", outline="#555")
        self.lamp.create_text(38, 13, text="IN1", fill="#ccc")
        self.lamp_in2 = self.lamp.create_oval(66, 5, 82, 21, fill="#333", outline="#555")
        self.lamp.create_text(98, 13, text="IN2", fill="#ccc")
        self.dir_label = ttk.Label(mot, text="정지", width=22)
        self.dir_label.grid(row=0, column=1, sticky="w")
        self.pwm_bar = tk.Canvas(mot, width=260, height=16, highlightthickness=1,
                                 highlightbackground="#555", bg="#22262C")
        self.pwm_bar.grid(row=1, column=1, sticky="w", pady=(4, 0))
        self.pwm_fill = self.pwm_bar.create_rectangle(0, 0, 0, 16, fill="#4CAF50", width=0)
        self.pwm_text = self.pwm_bar.create_text(130, 8, text="PWM 0 / 255", fill="#fff")

        # Run controls
        run = ttk.LabelFrame(left, text="실행", padding=8)
        run.grid(row=3, column=0, sticky="ew", pady=(10, 0))
        ttk.Checkbutton(run, text="일시정지", variable=self.paused).grid(row=0, column=0, sticky="w")
        ttk.Button(run, text="1프레임 진행", command=self._single_step).grid(row=0, column=1, padx=4)
        ttk.Button(run, text="PNG 저장", command=self._save_png).grid(row=0, column=2, padx=4)
        ttk.Checkbutton(run, text="IMU 정상", variable=self.imu_ok).grid(row=1, column=0, sticky="w")
        ttk.Checkbutton(run, text="GPS 정상", variable=self.gps_ok).grid(row=1, column=1, sticky="w")
        ttk.Button(run, text="재부팅", command=self.reset).grid(row=1, column=2, padx=4)
        ttk.Label(run, text="패널 색").grid(row=2, column=0, sticky="w", pady=(6, 0))
        cb = ttk.Combobox(run, textvariable=self.scheme, values=list(COLOR_SCHEMES),
                          state="readonly", width=20)
        cb.grid(row=2, column=1, columnspan=2, sticky="w", pady=(6, 0))
        cb.bind("<<ComboboxSelected>>", lambda e: self._redraw())
        ttk.Checkbutton(run, text="픽셀 격자", variable=self.grid_on,
                        command=self._draw_grid).grid(row=3, column=0, sticky="w")
        self.status = ttk.Label(run, text="", foreground="#666")
        self.status.grid(row=4, column=0, columnspan=3, sticky="w", pady=(6, 0))

        # Sensors
        pre = ttk.LabelFrame(right, text="프리셋", padding=8)
        pre.grid(row=0, column=0, sticky="ew")
        ttk.Button(pre, text="정지 (수평)", command=self.preset_still).grid(row=0, column=0, padx=3)
        ttk.Button(pre, text="기울임 (피치=위도)", command=self.preset_tilt).grid(row=0, column=1, padx=3)
        ttk.Button(pre, text="회전 (wz 15 dps)", command=self.preset_spin).grid(row=0, column=2, padx=3)
        ttk.Checkbutton(pre, text="센서 잡음", variable=self.noise).grid(row=1, column=0, sticky="w", pady=(6, 0))
        ttk.Checkbutton(pre, text="wz로 지자기 회전", variable=self.spin_mag).grid(row=1, column=1, sticky="w", pady=(6, 0))
        ttk.Checkbutton(pre, text="GPS 시각 자동 진행", variable=self.gps_auto).grid(row=1, column=2, sticky="w", pady=(6, 0))

        gps = ttk.LabelFrame(right, text="GPS", padding=8)
        gps.grid(row=1, column=0, sticky="ew", pady=(10, 0))
        self._fields(gps, GPS_FIELDS, cols=2)
        imu = ttk.LabelFrame(right, text="IMU (펌웨어 기준 좌표·단위)", padding=8)
        imu.grid(row=2, column=0, sticky="ew", pady=(10, 0))
        self._fields(imu, IMU_FIELDS, cols=3)

    def _fields(self, parent, fields, cols):
        for i, (key, label, default) in enumerate(fields):
            r, c = divmod(i, cols)
            var = tk.StringVar(value=str(default))
            self.vars[key] = var
            ttk.Label(parent, text=label).grid(row=r * 2, column=c, sticky="w", padx=4)
            sb = ttk.Spinbox(parent, textvariable=var, width=14,
                             from_=-1e6, to=1e6, increment=1 if key in INT_KEYS else 0.1)
            sb.grid(row=r * 2 + 1, column=c, sticky="w", padx=4, pady=(0, 4))

    def _draw_grid(self):
        for item in self.grid_items:
            self.canvas.delete(item)
        self.grid_items = []
        if not self.grid_on.get() or self.scale < 3:
            return
        s = self.scale
        w, h = oled_render.WIDTH * s, oled_render.HEIGHT * s
        for x in range(s - 1, w, s):
            self.grid_items.append(self.canvas.create_line(x, 0, x, h, fill=GRID_COLOR))
        for y in range(s - 1, h, s):
            self.grid_items.append(self.canvas.create_line(0, y, w, y, fill=GRID_COLOR))

    def _bind_keys(self):
        for key, d in (("<Left>", -1), ("<Up>", -1), ("<Right>", 1), ("<Down>", 1)):
            self.root.bind(key, lambda e, d=d: self._key(lambda: self.rotate(d)))
        for key in ("<Return>", "<space>"):
            self.root.bind(key, lambda e: self._key(self.click))
        self.root.bind("<MouseWheel>", self._on_wheel_global)

    def _key(self, action):
        # Arrow keys keep their usual meaning while editing a sensor field.
        focus = self.root.focus_get()
        if isinstance(focus, (tk.Entry, ttk.Entry, ttk.Spinbox, ttk.Combobox)):
            return
        action()
        return "break"

    # -------------------------------------------------------------- inputs
    def rotate(self, steps):
        self.client.rotate(steps)

    def click(self):
        self.client.click()

    def _on_canvas_click(self, event):
        self.canvas.focus_set()   # leave any sensor field so the keys drive the rotary
        self.click()

    def _on_wheel(self, event):
        # Windows: event.delta is a multiple of 120 per notch; wheel down = CW
        self.wheel_acc += event.delta
        steps = int(self.wheel_acc / 120)
        if steps:
            self.wheel_acc -= steps * 120
            self.rotate(-steps)
        return "break"

    def _on_wheel_global(self, event):
        if event.widget is self.canvas:
            return self._on_wheel(event)

    def reset(self):
        self.client.reset(self.imu_ok.get(), self.gps_ok.get())

    # ------------------------------------------------------------- sensors
    def _num(self, key):
        try:
            v = float(self.vars[key].get())
        except ValueError:
            return None
        return int(round(v)) if key in INT_KEYS else v

    def _set(self, key, value):
        self.vars[key].set(("%d" % value) if key in INT_KEYS else ("%.6f" % value).rstrip("0").rstrip("."))

    def preset_still(self):
        for k, v in (("ax", 0), ("ay", 0), ("az", G), ("wx", 0), ("wy", 0), ("wz", 0),
                     ("mx", 20), ("my", 0), ("mz", -40)):
            self._set(k, v)
        self.spin_mag.set(False)

    def preset_tilt(self):
        lat = self._num("lat") or 37.5665
        th = math.radians(abs(lat))
        # pitch = asin(-ax/g): pitch the mount up by the latitude (polar alignment)
        self._set("ax", -G * math.sin(th))
        self._set("ay", 0)
        self._set("az", G * math.cos(th))
        for k in ("wx", "wy", "wz"):
            self._set(k, 0)
        self.spin_mag.set(False)

    def preset_spin(self):
        self._set("wx", 0.3)
        self._set("wy", -0.2)
        self._set("wz", 15)
        self.spin_mag.set(True)

    def _advance_sensors(self):
        """Per-frame changes: GPS clock and the rotating magnetic vector."""
        if self.gps_auto.get():
            try:
                t = datetime.datetime(2000 + self._num("year"), self._num("month"), self._num("day"),
                                      self._num("hour"), self._num("minute"), self._num("seconds"),
                                      self._num("milliseconds") * 1000)
                t += datetime.timedelta(milliseconds=FRAME_MS)
                for k, v in (("year", t.year - 2000), ("month", t.month), ("day", t.day),
                             ("hour", t.hour), ("minute", t.minute), ("seconds", t.second),
                             ("milliseconds", t.microsecond // 1000)):
                    self._set(k, v)
            except (TypeError, ValueError):
                pass
        if self.spin_mag.get():
            mx, my, wz = self._num("mx"), self._num("my"), self._num("wz")
            if None not in (mx, my, wz):
                r = math.hypot(mx, my)
                h = math.atan2(-my, mx) + math.radians(wz) * FRAME_MS / 1000.0
                self._set("mx", r * math.cos(h))
                self._set("my", -r * math.sin(h))

    def _sensor_message(self):
        gps = {}
        for key, _, _ in GPS_FIELDS:
            v = self._num(key)
            if v is not None:
                gps[key] = v
        imu = {}
        for key, _, _ in IMU_FIELDS:
            v = self._num(key)
            if v is not None:
                imu[key] = v
        if self.noise.get():
            for keys, sigma in ((("ax", "ay", "az"), 0.02), (("wx", "wy", "wz"), 0.05),
                                (("mx", "my", "mz"), 0.3), (("temp",), 0.05)):
                for k in keys:
                    if k in imu:
                        imu[k] += random.gauss(0.0, sigma)
        return gps, imu

    # ---------------------------------------------------------- main loop
    def _send_frame_request(self):
        self._advance_sensors()
        gps, imu = self._sensor_message()
        self.client.sensors(gps, imu)
        self.client.tick()

    def _tick_loop(self):
        if not self.paused.get():
            try:
                self._send_frame_request()
            except OSError:
                self.status.config(text="러너와 연결이 끊어졌습니다.")
                return
        # drift-free 10 Hz schedule
        self._next_tick += FRAME_MS / 1000.0
        delay = self._next_tick - time.perf_counter()
        if delay < -0.5:   # fell far behind (e.g. window dragged): resync
            self._next_tick = time.perf_counter()
            delay = 0
        self.root.after(max(1, int(delay * 1000)), self._tick_loop)

    def _single_step(self):
        self.paused.set(True)
        self._send_frame_request()

    def _poll(self):
        latest = None
        try:
            while True:
                msg = self.client.messages.get_nowait()
                t = msg.get("type")
                if t == "frame":
                    latest = msg
                elif t == "error":
                    self.status.config(text="러너 오류: %s" % msg.get("msg"))
                elif t == "eof":
                    self.status.config(text="러너가 종료되었습니다.")
        except Exception:
            pass
        if latest is not None:
            self.last_frame = latest
            self._redraw()
        self.root.after(15, self._poll)

    def _redraw(self):
        f = self.last_frame
        if f is None:
            return
        px = oled_render.render_lines(f["lines"])
        top, rest = COLOR_SCHEMES[self.scheme.get()]
        rows = []
        for y, row in enumerate(px):
            on = top if y < 16 else rest
            rows.append("{" + " ".join(on if v else OFF_COLOR for v in row) + "}")
        self.base_img.put(" ".join(rows), to=(0, 0))
        self.zoom_img = self.base_img.zoom(self.scale)
        self.canvas.itemconfig(self.img_item, image=self.zoom_img)

        m = f["motor"]
        self.lamp.itemconfig(self.lamp_in1, fill="#FF5A4E" if m["in1"] else "#333")
        self.lamp.itemconfig(self.lamp_in2, fill="#4EC3FF" if m["in2"] else "#333")
        if m["in1"] and m["in2"]:
            d = "브레이크 (IN1=IN2=HIGH)"
        elif m["in2"]:
            d = "정방향 (+, IN2)"
        elif m["in1"]:
            d = "역방향 (-, IN1)"
        else:
            d = "정지/관성 (IN1=IN2=LOW)"
        self.dir_label.config(text=d)
        self.pwm_bar.coords(self.pwm_fill, 0, 0, 260 * m["pwm"] / 255.0, 16)
        self.pwm_bar.itemconfig(self.pwm_text, text="PWM %d / 255" % m["pwm"])
        self.status.config(text="frame %d   t=%.1f s" % (f["seq"], f["t_ms"] / 1000.0))

    def _save_png(self, path=None):
        if path is None:
            path = filedialog.asksaveasfilename(defaultextension=".png",
                                                filetypes=[("PNG", "*.png")])
            if not path:
                return
        self.zoom_img.write(path, format="png")

    def quit(self):
        self.client.close()
        self.root.destroy()


# ------------------------------------------------------------ scripted tour
TOUR = [
    # (action, arg, screenshot name or None)
    ("wait", 3, "00_boot_partial"),
    ("wait_title", "Main Menu", "01_main_menu"),
    ("window", None, "window_main_menu"),
    ("click", None, "02_ra_speed"),
    ("click", None, None), ("rotate", 25, "03_ra_speed_edit"),
    ("window", None, "window_ra_speed_edit"),
    ("click", None, None), ("rotate", 2, None), ("click", None, None),
    ("rotate", 1, None), ("click", None, "04_status_gps"), ("click", None, None),
    ("preset", "tilt", None),
    ("rotate", 1, None), ("click", None, "05_status_acc_gyro"), ("click", None, None),
    ("rotate", 1, None), ("click", None, "06_status_mag_alt"), ("click", None, None),
    ("rotate", 1, None), ("click", None, "07_status_imu_calib"), ("click", None, None),
    ("rotate", 1, None), ("click", None, "08_do_imu_calib"),
    ("rotate", 2, None), ("click", None, "09_calib_gyro_bias"), ("click", None, None),
    ("rotate", 3, "10_do_imu_calib_quit"), ("click", None, "11_main_after_calib"),
    ("rotate", 1, None), ("click", None, "12_att_estimator"), ("click", None, None),
    ("rotate", 1, None), ("click", None, "13_health"), ("click", None, None),
    ("rotate", 1, None), ("click", None, "14_axis_for_moon"), ("click", None, None),
    ("rotate", 1, None), ("click", None, "15_planet_menu"),
    ("rotate", 2, None), ("click", None, "16_planet_mars"), ("click", None, "17_planet_mars_tracking"),
    ("rotate", 1, None), ("click", None, None), ("rotate", 3, None), ("click", None, None),
    ("rotate", 1, None), ("click", None, "18_sleep"), ("click", None, "19_back_to_main"),
]


def run_tour(sim, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    sim.paused.set(True)            # step frames ourselves, deterministically
    steps = list(TOUR)
    log = []

    def frame():
        sim._send_frame_request()
        f = sim.client.wait_for("frame")
        sim.last_frame = f
        sim._redraw()
        return f

    def do_next():
        if not steps:
            with open(os.path.join(out_dir, "tour_log.txt"), "w", encoding="utf-8") as fh:
                fh.write("\n".join(log) + "\n")
            sim.quit()
            return
        action, arg, shot = steps.pop(0)
        if action == "wait":
            for _ in range(arg):
                f = frame()
        elif action == "wait_title":
            for _ in range(100):
                f = frame()
                if f["lines"][1] == arg:
                    break
        elif action == "rotate":
            sim.rotate(arg)
            f = frame()
        elif action == "click":
            sim.click()
            f = frame()
        elif action == "preset":
            getattr(sim, "preset_" + arg)()
            f = frame()
        elif action == "window":
            sim.root.update()
            grab_window(sim.root, os.path.join(out_dir, shot + ".png"))
            sim.root.after(50, do_next)
            return
        sim.root.update_idletasks()
        if shot:
            sim._save_png(os.path.join(out_dir, shot + ".png"))
            log.append("== %s\n%s\nmotor=%s" % (shot, "\n".join(f["lines"]), f["motor"]))
        sim.root.after(30, do_next)

    sim.root.after(500, do_next)


def grab_window(root, path):
    """Screenshot of the whole simulator window (Windows, via PowerShell)."""
    root.lift()
    root.attributes("-topmost", True)
    root.update()
    time.sleep(0.3)
    x, y = root.winfo_rootx(), root.winfo_rooty()
    w, h = root.winfo_width(), root.winfo_height()
    ps = ("Add-Type -AssemblyName System.Drawing;"
          "$b=New-Object System.Drawing.Bitmap %d,%d;"
          "$g=[System.Drawing.Graphics]::FromImage($b);"
          "$g.CopyFromScreen(%d,%d,0,0,$b.Size);"
          "$b.Save('%s',[System.Drawing.Imaging.ImageFormat]::Png)" % (w, h, x, y, path))
    subprocess.run(["powershell", "-NoProfile", "-Command", ps], check=False)
    root.attributes("-topmost", False)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--runner", default=DEFAULT_RUNNER, help="path to ui_runner.exe")
    ap.add_argument("--backend", default="firmware", help="UI backend inside the runner (default firmware)")
    ap.add_argument("--scale", type=int, default=5, help="pixel zoom (default 5)")
    ap.add_argument("--shots", metavar="DIR", help="run the scripted tour and save screenshots")
    args = ap.parse_args()

    try:
        # Crisp pixels on high-DPI Windows displays
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except Exception:
        pass
    root = tk.Tk()
    try:
        sim = Simulator(root, args.runner, args.scale, args.backend)
    except FileNotFoundError as e:
        print(e, file=sys.stderr)
        return 1
    if args.shots:
        run_tour(sim, args.shots)
    root.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
