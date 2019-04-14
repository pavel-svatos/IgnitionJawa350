/*
 * Ignition_Jawa350.h
 *
 *  Created on: Jun 30, 2017
 *      Author: pajas
 */

#ifndef IGNITION_JAWA350_H_
#define IGNITION_JAWA350_H_

  // type defines
  typedef signed char      si8;
  typedef unsigned char    ui8;
  typedef short           si16;
  typedef unsigned short  ui16;
  typedef int             si32;
  typedef unsigned int    ui32;

  // user configuration
  #define PIN_IGN_SENS      16   // D0
  #define L_IGN_PIN_OUT      5   // D1
  #define R_IGN_PIN_OUT      4   // D2

  #define ALL_IGN_CURVES     5
  #define ALL_PWR_CURVES     3

  #define _DEBUG             0

  #define IGN_IDLE_TIME 15000000u // ignition idle time 15s
  #define IN_LOCK_TIME      3000u // delay 3ms for next input reading to avoid a ignition(cable) interferences

  enum: ui8 {
    sideL = 0,
    sideR = 1
  };

  // output reversed logic
  // 0 --> spark On
  enum: ui8 {
    mosfetOff = 1,
    mosfetOn  = 0
  };

  typedef struct {
    ui8 idx;
    ui32 rpm;
    si8 offset;
    si16 ratio;
  } ignCurve_t;

  typedef struct {
    ui8 idx;
    ui32 rpm;
    si8 offset;
    si16 ratio;
  } pwrCurve_t;

  typedef struct {
    ui8  ignPinOut;
    bool f_ignCycle;
    ui32 delayOn;
    ui32 delayOff;
    ui32 ignCycleDur;
    ui32 ignTimeZero;
    ui32 lastZero;
  } i_t;

#endif
