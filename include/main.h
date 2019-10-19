/*
 * main.h
 *
 *  Created on: Oct 8, 2019
 *      Author: Pavel Svatos
 */
#include <c_types.h>
#include <ign_timer.h>

#ifndef MAIN_H_
#define MAIN_H_

  // user configuration
  #define PIN_IGN_SENS      16   // D0
  #define L_IGN_PIN_OUT      5   // D1
  #define R_IGN_PIN_OUT      4   // D2

  #define ALL_IGN_CURVES     5
  #define ALL_PWR_CURVES     3

  #define _DEBUG             0

  #define IDLE_TIME        1000000u   // mosfet max 1s ON
  #define DEAD_TIME_EDGE      1000u   // deadTimeDur after every input transition (H->L or L->H) the input is blocked for defined time
  #define DEAD_TIME_SPARK     3000u   // deadTimeDur after every spark  

  typedef struct {
    u8 idx;
    u32 rpm;
    s8 offset;
    s16 ratio;
  } ignCurve_t;

  typedef struct {
    u8 idx;
    u32 rpm;
    s8 offset;
    s16 ratio;
  } pwrCurve_t;

  typedef struct {
    u32 delaySpark;
    u32 delayCharge;
    u32 cycleDur;
    u32 zeroTime;
    u32 lastZero;
  } ignData_t;

#endif
