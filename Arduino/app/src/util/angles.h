#pragma once
#ifndef _ANGLES_H_
#define _ANGLES_H_

/************************************/
// Degree / radian constants shared by the pure C++ modules.
// (Arduino.h has PI, DEG_TO_RAD, RAD_TO_DEG macros; these names do not clash.)
/************************************/
namespace util {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr float  kDegToRadF = (float)kDegToRad;
constexpr float  kRadToDegF = (float)kRadToDeg;

}  // namespace util

#endif // _ANGLES_H_
