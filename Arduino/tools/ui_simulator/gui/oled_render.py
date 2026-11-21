"""Draw text lines onto a 128x64 SSD1306 frame buffer the way the firmware does.

OLED::print() does clearDisplay(), setCursor(0, 0) and println() for each of the
8 lines with text size 1, so this follows Adafruit_GFX::write():
  - each character is a 5x7 glyph in a 6x8 cell (transparent background),
  - '\\n' moves to the next 8-pixel row, '\\r' is ignored,
  - with wrap on (the default) a character that does not fit is drawn on the
    next row,
  - with the default _cp437 == false, codes >= 176 are shifted by one.
No tkinter here so it can be tested headless.
"""
from glcdfont_data import FONT

WIDTH = 128
HEIGHT = 64


def render_lines(lines):
    """Return a list of HEIGHT rows, each a list of WIDTH ints (0/1)."""
    px = [[0] * WIDTH for _ in range(HEIGHT)]
    x = y = 0
    text = "".join(line + "\n" for line in lines)
    for ch in text:
        c = ord(ch) & 0xFF
        if c == 0x0A:
            x = 0
            y += 8
            continue
        if c == 0x0D:
            continue
        if x + 6 > WIDTH:   # wrap
            x = 0
            y += 8
        if c >= 176:        # classic (non-CP437) glyph table quirk
            c = min(c + 1, 255)
        draw_char(px, x, y, c)
        x += 6
    return px


def draw_char(px, x0, y0, c):
    base = c * 5
    for col in range(5):
        bits = FONT[base + col]
        for row in range(8):
            if bits & (1 << row):
                x, y = x0 + col, y0 + row
                if 0 <= x < WIDTH and 0 <= y < HEIGHT:
                    px[y][x] = 1


def to_ascii(px):
    """Debug helper: frame buffer as text art."""
    return "\n".join("".join("#" if v else "." for v in row) for row in px)
