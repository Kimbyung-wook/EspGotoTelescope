#!/usr/bin/env python3
"""Code metrics for the refactoring log (docs/REFACTORING.md).

Counts non-blank, non-comment-only lines (LOC) of the firmware sources, per area,
plus the two main loops (app.ino, simulator firmware_ui.cpp) and the tests.

Usage:
  python Arduino/scripts/metrics.py            # working tree
  python Arduino/scripts/metrics.py <git-rev>  # a commit (e.g. dc3f8eb)
  python Arduino/scripts/metrics.py --md <rev>...  # one Markdown table row per rev
"""
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
APP = "Arduino/app"
SRC = APP + "/src/"
SKIP = {"Arduino/app/src/remote/remote_page.h"}       # generated from web/index.html
EXT = (".h", ".cpp", ".ino")
LOOPS = ["Arduino/app/app.ino", "Arduino/tools/ui_simulator/runner/firmware_ui.cpp"]
TESTS = ("Arduino/test/", "Arduino/tools/ui_simulator/tests/")


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True, check=True,
                          text=True, encoding="utf-8").stdout


def files(rev):
    if rev:
        names = git("ls-tree", "-r", "--name-only", rev, "Arduino").splitlines()
    else:
        names = []
        for d, _, fs in os.walk(os.path.join(ROOT, "Arduino")):
            for f in fs:
                names.append(os.path.relpath(os.path.join(d, f), ROOT).replace("\\", "/"))
    return [n for n in names if n.endswith(EXT) and "/build/" not in n and "/archive/" not in n]


def read(rev, name):
    if rev:
        return git("show", f"{rev}:{name}")
    with open(os.path.join(ROOT, name), encoding="utf-8", errors="replace") as fh:
        return fh.read()


def loc(text):
    n, block = 0, False
    for line in text.splitlines():
        s = line.strip()
        if block:
            if "*/" in s:
                block = False
                s = s.split("*/", 1)[1].strip()
            else:
                continue
        if s.startswith("/*") and "*/" not in s:
            block = True
            continue
        if not s or s.startswith("//") or re.fullmatch(r"/\*.*\*/", s):
            continue
        n += 1
    return n


def measure(rev=None):
    names = files(rev)
    m = {"areas": {}, "files": 0, "loc": 0, "largest": ("", 0), "loops": 0, "tests": 0}
    for n in names:
        if n in SKIP:
            continue
        if n.startswith(SRC):
            c = loc(read(rev, n))
            area = n[len(SRC):].split("/")[0] if "/" in n[len(SRC):] else "(top)"
            m["areas"][area] = m["areas"].get(area, 0) + c
            m["files"] += 1
            m["loc"] += c
            if c > m["largest"][1]:
                m["largest"] = (n[len(SRC):], c)
        elif n.startswith(TESTS):
            m["tests"] += loc(read(rev, n))
    for n in LOOPS:
        if n in names:
            m["loops"] += loc(read(rev, n))
    return m


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    args = sys.argv[1:]
    if args and args[0] == "--md":
        print("| rev | src 파일 | src LOC | ui | app | astro | 가장 큰 파일 | 루프 2벌 | 테스트 LOC |")
        print("|---|---|---|---|---|---|---|---|---|")
        for rev in args[1:] or [None]:
            m = measure(None if rev == "." else rev)
            a = m["areas"]
            print(f"| {rev or '.'} | {m['files']} | {m['loc']} | {a.get('ui', 0)} | {a.get('app', 0)} "
                  f"| {a.get('astro', 0)} | `{m['largest'][0]}` {m['largest'][1]} | {m['loops']} | {m['tests']} |")
        return
    m = measure(args[0] if args else None)
    print(f"src files {m['files']}  LOC {m['loc']}  (remote_page.h, archive/ excluded)")
    for k in sorted(m["areas"]):
        print(f"  {k:12s} {m['areas'][k]:6d}")
    print(f"largest     {m['largest'][0]} ({m['largest'][1]})")
    print(f"main loops  {m['loops']}  (app.ino + simulator firmware_ui.cpp)")
    print(f"tests       {m['tests']}")


if __name__ == "__main__":
    main()
