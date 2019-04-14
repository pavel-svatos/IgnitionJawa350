#include <string.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include "Ignition_Jawa350.h"

ignCurve_t ignCurve[ALL_IGN_CURVES] = {
  {0,     0,  3,     69},
  {1,   900,  0,     54},
  {2,  1300, 21,    357},
  {3,  2400, 30,   -916},
  {4,  5500, 24, -30000}
};

pwrCurve_t pwrCurve[ALL_PWR_CURVES] = {
  {0,     0, 75,    -48},
  {1,  1200, 45,    240},
  {2,  5800, 55,    400}
};

bool f_ignIdle     = false;
ui8  ignIn         = 0;
ui8  lastIgnIn     = 0;
ui32 angle         = 0;
ui32 rpm           = 60;
ui32 rotDur        = 1000000;
float volt         = 0.0;

ui32 initTime = micros();

i_t i[2] = {
  { L_IGN_PIN_OUT, false, 100000u, 200000u, 1000000u, initTime, initTime - 1000000u},
  { R_IGN_PIN_OUT, false, 100000u, 200000u, 1000000u, initTime, initTime - 1000000u},
};

#if _DEBUG
  ui32 debugCnt = 0;
  bool f_printOut = false;
#endif

void calcRpm(void) {
  if ( (rotDur > 1000000) || (rpm < 60)) {
    rpm = 60;
  } else if (rotDur < 6000) {
    rpm = 10000;
  } else {
    rpm = 60000000 / rotDur;
  }
}

void calcIgnData(i_t& i) {
  ui8 _idx = ALL_IGN_CURVES;
  for(;_idx--;) {
    if (rpm > ignCurve[_idx].rpm) break;
  }
  angle = ( (ignCurve[_idx].offset * 100) + ((si32)(rpm * 100) / ignCurve[_idx].ratio) );
  i.delayOn  = (rotDur * (4500 - angle)) / 36000u; // base angle = 45°- offset, start of spark

  _idx = ALL_PWR_CURVES;
  for(;_idx--;) {
    if (rpm > pwrCurve[_idx].rpm) break;
  }
  ui32 procPwr = (pwrCurve[_idx].offset * 100) + ( (si32)(rpm * 100) / pwrCurve[_idx].ratio);
  i.delayOff = i.delayOn + ( (rotDur * procPwr) / 10000 );
  #if _DEBUG
    if (f_printOut) {
      Serial.printf("procPwr: %d\t", procPwr);
      f_printOut = false;
    }
  #endif
}

float getVoltage(void) {
  return (analogRead(A0)/1023.0)*10.2;
}

void calcCycleDur(i_t& i) {
  i.lastZero = i.ignTimeZero;
  i.ignTimeZero  = micros();
  ui32 _diff = i.ignTimeZero - i.lastZero;
  rotDur = _diff < 1000000 ? _diff : _diff = 1000000;
  calcRpm();
  i.f_ignCycle = true;
  f_ignIdle    = false;
  calcIgnData(i);
}

void setMosfet(i_t& i) {
  // process ignition cycle
  i.ignCycleDur = micros() - i.ignTimeZero;
  if (i.ignCycleDur > i.delayOff) {
    digitalWrite(i.ignPinOut, mosfetOn);
    i.f_ignCycle = false;
  } else if (i.ignCycleDur > i.delayOn) {
    digitalWrite(i.ignPinOut, mosfetOff);
  }
}

bool ignStateHandler(void) {
  ui32 _curTime = micros();
  ui32 _idleDur = 0;
  bool _f_inLocked = false;
  if (!f_ignIdle) {
    // get actual idle time of both sides and set shorter
    _idleDur = _curTime - i[sideL].ignTimeZero;
    ui32 _R_idleDur = _curTime - i[sideR].ignTimeZero;
    if (_R_idleDur < _idleDur) {
      _idleDur = _R_idleDur;
    }
    if (_idleDur < IN_LOCK_TIME) {
      // the maximal input sample rate could be every 3ms (max engine rpm cca. 10000)
      // after every input transition (H->L or L->H) the input is blocked for defined time
      // this eliminate an interferences invoked by ignition
      _f_inLocked = true;
    } else if (_idleDur > IGN_IDLE_TIME) {
      // after defined time the ignition is disconnected of power save reason
      digitalWrite(L_IGN_PIN_OUT, mosfetOff);
      digitalWrite(R_IGN_PIN_OUT, mosfetOff);
      f_ignIdle = true;
      // update SSID
      volt = getVoltage();
      String s = String(volt);
      s = "CurrentVoltage: " + s + "V)";
      WiFi.softAP((char*)&s[0], "", 4, 0);
    }
  }
  return _f_inLocked;
}

void setup() {
  pinMode(PIN_IGN_SENS,  INPUT);
  digitalWrite(L_IGN_PIN_OUT, mosfetOn);
  digitalWrite(R_IGN_PIN_OUT, mosfetOn);
  pinMode(L_IGN_PIN_OUT, OUTPUT);
  pinMode(R_IGN_PIN_OUT, OUTPUT);
  pinMode(A0, INPUT); // voltage measurement
  delay(500);
  volt = getVoltage();
  String s = String(volt);
  s = "Jawa350 is the best! (" + s + "V)";
  ignIn = (ui8)digitalRead(PIN_IGN_SENS);
  lastIgnIn = ignIn;
  Serial.begin(115200);
  Serial.println("\nConfiguring access point...");
  WiFi.softAP((char*)&s[0], "", 4, 0);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println("");
}

//          | < ---- counted from last half revDur -----------------> |
//   f_ignCycle start <---------- ignCycleDur -----> f_ignCycle stop  |
//          |                      |            |                     |
//      time ZERO    delay      delayOn      delayOff       again time ZERO
//          |                      | mosfet off |                     |
//----------|----------------------|------------|------------//-------|-----

void loop() {
  // if input not locked...
  if (!ignStateHandler()) {
    ignIn = (ui8)digitalRead(PIN_IGN_SENS);
  }

  if (i[sideL].f_ignCycle) {
    setMosfet(i[sideL]);
  }
  if (i[sideR].f_ignCycle) {
    setMosfet(i[sideR]);
  }

  // waiting for start of ignition cycle - rising or falling edge
  if (!lastIgnIn && ignIn) {
    calcCycleDur(i[sideL]);
  }
  if (lastIgnIn && !ignIn) {
    calcCycleDur(i[sideR]);
  }

  lastIgnIn = ignIn;

  #if _DEBUG
    if (!(debugCnt % 100000)) {
      Serial.printf("rotDur: %d \t rpm: %d\t angle: %d\t Delays: %d\t %d\t %d\t %d\n", rotDur, rpm, angle, i[sideL].delayOn, i[sideL].delayOff, i[sideR].delayOn, i[sideR].delayOff);
      f_printOut = true;
    }
    debugCnt++;
  #endif
}
