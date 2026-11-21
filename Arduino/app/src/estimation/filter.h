#pragma once
#ifndef _FILTER_H_
#define _FILTER_H_

// Fixed-step filters : dt is set in initialize(). The dt argument of update()
// is kept for API compatibility and is not used.
class LPF{
public:
  LPF(){};
  ~LPF(){};

public:
  void initialize(float tau_in = 1.0f, float dt_in = 0.1f);
  void setTau(float tau_in);
  float getTau();
  float getState();
  float update(float u_in, float dt_in = 0.1f);

private:
  float x;
  float x_prev;
  float tau;
  float gamma; // exp(-dt/tau)
  bool  first = true;
  float dt;
};

class HPF{
public:
  HPF(){};
  ~HPF(){};

public:
  void initialize(float tau_in = 1.0f, float dt_in = 0.1f);
  void setTau(float tau_in);
  float getTau();
  float getState();
  float update(float u_in, float dt_in = 0.1f);

private:
  float x;
  float x_prev;
  float u_prev;
  float tau;
  float gamma; // exp(-dt/tau)
  bool  first = true;
  float dt;
};

class ComplementaryFilter{
public:
  ComplementaryFilter(){};
  ~ComplementaryFilter(){};

public:
  void initialize(float tau_in = 1.0f, float dt_in = 0.1f);
  float update(float x_low, float x_high, float dt_in);
  void setTau(float tau_in);
  float getTau();
  float getXhigh();
  float getXlow();

private:
  float x;
  float tau;
  float dt;
  LPF lpf_L;
  LPF lpf_H;
};

#endif // _FILTER_H_