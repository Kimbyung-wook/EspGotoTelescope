#include "oled.h"
#include "../board_config.h"

static const int SCREEN_WIDTH  = 128;   // [pixel]
static const int SCREEN_HEIGHT = 64;
static const int OLED_RESET    = -1;    // no reset pin (shares the board reset)

Oled::Oled() : device_(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

bool Oled::begin() {
  Wire.setClock(400 * 1000);
  const bool ok = device_.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
  device_.clearDisplay();
  delay(100);

  device_.setTextSize(1);                 // normal 1:1 pixel scale
  device_.setTextColor(SSD1306_WHITE);
  device_.setCursor(0, 0);
  device_.println(F("Display Initialize"));

  delay(100);
  device_.display();
  return ok;
}

void Oled::setPower(bool on) {
  if (on == power_on_) return;
  device_.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
  power_on_ = on;
}

void Oled::print(const char lines[ROWS][COLS]) {
  print_time_.start();
  device_.clearDisplay();
  device_.setCursor(0, 0);                // top-left corner
  for (int i = 0; i < ROWS; i++) device_.println(lines[i]);
  device_.display();
  print_time_.stop();
}
