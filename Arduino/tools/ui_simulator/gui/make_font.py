"""Convert Adafruit GFX glcdfont.c into glcdfont_data.py.

Usage:
    python make_font.py [path/to/glcdfont.c]

Default path is the Arduino library installed for this user:
    %USERPROFILE%/Documents/Arduino/libraries/Adafruit_GFX_Library/glcdfont.c
The BSD license text next to glcdfont.c (license.txt) is copied into the
generated file, as the license requires.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT = os.path.join(os.path.expanduser("~"), "Documents", "Arduino", "libraries",
                       "Adafruit_GFX_Library", "glcdfont.c")


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else DEFAULT
    with open(src, encoding="latin-1") as f:
        text = f.read()
    body = text[text.index("font[]"):]
    body = body[body.index("{") + 1:body.index("};")]
    body = re.sub(r"//[^\n]*", "", body)
    data = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]{2}", body)]
    if len(data) != 256 * 5:
        raise SystemExit("expected 1280 bytes, got %d" % len(data))

    lic_path = os.path.join(os.path.dirname(src), "license.txt")
    with open(lic_path, encoding="latin-1") as f:
        lic = f.read().strip()

    out = os.path.join(HERE, "glcdfont_data.py")
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write('"""Adafruit GFX classic 5x7 font (glcdfont.c), converted by make_font.py.\n\n')
        f.write("256 glyphs x 5 column bytes; bit 0 of each byte is the top pixel.\n\n")
        f.write("Original file: Adafruit_GFX_Library/glcdfont.c, distributed under:\n\n")
        for line in lic.splitlines():
            f.write(("    " + line).rstrip() + "\n")
        f.write('"""\n\n')
        f.write("FONT = bytes([\n")
        for i in range(0, len(data), 15):
            f.write("    " + ", ".join("0x%02X" % b for b in data[i:i + 15]) + ",\n")
        f.write("])\n")
    print("wrote", out)


if __name__ == "__main__":
    main()
