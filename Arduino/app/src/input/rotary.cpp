#include "rotary.h"

Rotary* Rotary::instance_ = nullptr;

Rotary::Rotary(uint8_t pin_s1, uint8_t pin_s2, uint8_t pin_key)
  : pin_s1_(pin_s1), pin_s2_(pin_s2), pin_key_(pin_key) {}

void Rotary::begin() {
  instance_ = this;
  pinMode(pin_s1_,  INPUT);
  pinMode(pin_s2_,  INPUT);
  pinMode(pin_key_, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_s1_),  Rotary::isrS1,     FALLING);
  attachInterrupt(digitalPinToInterrupt(pin_s2_),  Rotary::isrS2,     FALLING);
  attachInterrupt(digitalPinToInterrupt(pin_key_), Rotary::isrButton, RISING);
}

int Rotary::count() {
  noInterrupts();
  int c = count_;
  interrupts();
  return c;
}

bool Rotary::takeClick() {
  noInterrupts();
  bool c = click_pending_;
  click_pending_ = false;
  interrupts();
  return c;
}

int Rotary::clickCount() {
  noInterrupts();
  int c = clicks_;
  interrupts();
  return c;
}

IRAM_ATTR void Rotary::onEdge(int channel) {
  int step = decoder_.onEdge(channel, digitalRead(pin_s1_), digitalRead(pin_s2_), millis());
  count_ = count_ + step;
}

IRAM_ATTR void Rotary::onButton() {
  if (decoder_.onButton(millis())) {
    click_pending_ = true;
    clicks_ = clicks_ + 1;
  }
}

IRAM_ATTR void Rotary::isrS1()     { if (instance_) instance_->onEdge(0); }
IRAM_ATTR void Rotary::isrS2()     { if (instance_) instance_->onEdge(1); }
IRAM_ATTR void Rotary::isrButton() { if (instance_) instance_->onButton(); }
