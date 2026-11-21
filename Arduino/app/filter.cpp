#include "filter.h"
#include <math.h>

void  LPF::initialize(float tau_in = 1.00f, float dt_in = 0.100f){
  tau = tau_in;
  dt = dt_in;
  gamma = expf(-(float)dt / tau);
}
void  LPF::setTau(float tau_in){
  tau = tau_in;
  gamma = expf(-(float)dt / tau);
}
float LPF::getTau(){
  return tau;
}

float LPF::getState(){
  return x;
}
float LPF::update(float u_in, float dt_in = 0.100f){
  x = (1.0f - gamma) * x_prev + gamma * u_in;
  x_prev = x;
  return x;
}


void  HPF::initialize(float tau_in, float dt_in = 0.100f){
  tau = tau_in;
  dt = dt_in;
  gamma = expf(-(float)dt / tau);
}
void  HPF::setTau(float tau_in){
  tau = tau_in;
  gamma = expf(-(float)dt / tau);
}
float HPF::getTau(){
  return tau;
}
float HPF::getState(){
  return x;
}
float HPF::update(float u_in, float dt_in = 0.100f){
  x = (1.0f - gamma) * x_prev + (u_in - u_prev);
  x_prev = x;
  u_prev = u_in;
  return x;
}

void ComplementaryFilter::initialize(float tau_in = 1.00f, float dt_in = 0.100f){
  tau = tau_in;
  dt = dt_in;
  lpf_L.initialize(tau, dt);
  lpf_H.initialize(tau, dt);
}
float ComplementaryFilter::update(float u_low, float u_high, float dt_in){
  lpf_L.update(u_low, dt_in);
  lpf_H.update(u_high*tau, dt_in);
  x = lpf_L.getState() + lpf_H.getState();
  return x;
}
void ComplementaryFilter::setTau(float tau_in){
  tau = tau_in;
  lpf_L.setTau(tau);
  lpf_H.setTau(tau);
}
float ComplementaryFilter::getTau(){
  return tau;
}
float ComplementaryFilter::getXhigh(){
  return lpf_H.getState();
}
float ComplementaryFilter::getXlow(){
  return lpf_L.getState();
}