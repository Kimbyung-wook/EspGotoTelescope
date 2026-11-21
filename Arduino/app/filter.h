#pragma once
#ifndef _FILTER_H_
#define _FILTER_H_

class LPF{
public:
  LPF(){};
  ~LPF(){};

public:
  void initialize(float tau_in, float dt_in);
  void setTau(float tau_in);
  float getTau();
  float getState();
  float update(float u_in, float dt_in);

private:
  float x;
  float x_prev;
  float tau;
  float gamma;
  float dt;
};

class HPF{
public:
  HPF(){};
  ~HPF(){};

public:
  void initialize(float tau_in, float dt_in);
  void setTau(float tau_in);
  float getTau();
  float getState();
  float update(float u_in, float dt_in);

private:
  float x;
  float x_prev;
  float u_prev;
  float tau;
  float gamma; // exp(-dt/tau);
  float dt;
};

class ComplementaryFilter{
public:
  ComplementaryFilter(){};
  ~ComplementaryFilter(){};

public:
  void initialize(float tau_in, float dt_in);
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