#include <Arduino.h>
#include <string.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <main.h>

ignCurve_t ignCurve[ALL_IGN_CURVES] = {
  {0,     0,  3,     69},
  {1,   900,  0,     54},
  {2,  1300, 21,    357},
  {3,  2400, 30,   -916},
  {4,  5500, 24, -30000}
};

pwrCurve_t pwrCurve[ALL_PWR_CURVES] = {
  {0,     0, 45,     10},
  {1,  1200, 35,    240},
  {2,  5800, 55,    400}
};

u8  ignIn           = 0;
u8  lastIgnIn       = 0;
u32 angle           = 0;
u32 rpm             = 60;
u32 rotDur          = 1000000;
float volt          = 0.0;

ignData_t ign = { 117000u, 700000u, 1000000u, micros(), micros() - 1000000u};

ignTimer tmr_L;
ignTimer tmr_R;
ignTimer tmrDeadTime;

inline void idle(void) {
  digitalWrite(L_IGN_PIN_OUT, 1);
  digitalWrite(R_IGN_PIN_OUT, 1);
  #if _DEBUG
    Serial.printf("Idle state\n");
  #endif
}
ignTimer tmrIdle(IDLE_TIME, &idle);

inline void charge_L(void) {
  digitalWrite(L_IGN_PIN_OUT, 0);
}
inline void charge_R(void) {
  digitalWrite(R_IGN_PIN_OUT, 0);
}

inline void spark_L(void) {
  digitalWrite(L_IGN_PIN_OUT, 1);
  if(rpm < 240) {
    delay(2);
    digitalWrite(L_IGN_PIN_OUT, 0);
    delay(2);
    digitalWrite(L_IGN_PIN_OUT, 1);
    delay(2);
    digitalWrite(L_IGN_PIN_OUT, 0);
    delay(2);
    digitalWrite(L_IGN_PIN_OUT, 1);
  }
  tmrDeadTime.set(DEAD_TIME_SPARK);
  tmr_L.set(ign.delayCharge, &charge_L);
}
inline void spark_R(void) {
  digitalWrite(R_IGN_PIN_OUT, 1);
  if(rpm < 240) {
    delay(2);
    digitalWrite(R_IGN_PIN_OUT, 0);
    delay(2);
    digitalWrite(R_IGN_PIN_OUT, 1);
    delay(2);
    digitalWrite(R_IGN_PIN_OUT, 0);
    delay(2);
    digitalWrite(R_IGN_PIN_OUT, 1);
  }
  tmrDeadTime.set(DEAD_TIME_SPARK);
  tmr_R.set(ign.delayCharge, &charge_R);
}

#if _DEBUG
  u32 debugCnt = 0;
  bool f_printOut = false;
#endif

inline void calcRpm(void) {
  if( (rotDur > 1000000) || (rpm < 60)) {
    rpm = 60;
  } else if(rotDur < 6000) {
    rpm = 10000;
  } else {
    rpm = 60000000 / rotDur;
  }
}

inline void calcIgnData(void) {
  u8 _idx = ALL_IGN_CURVES;
  while(_idx--) {
    if(rpm > ignCurve[_idx].rpm) 
      break;
  }
  angle = ( (ignCurve[_idx].offset * 100) + ((s32)(rpm * 100) / ignCurve[_idx].ratio) );
  ign.delaySpark  = (rotDur * (4500 - angle)) / 36000u; // base angle = 45°- offset, start of spark

  _idx = ALL_PWR_CURVES;
  while(_idx--) {
    if(rpm > pwrCurve[_idx].rpm)
      break;
  }
  u32 procPwr = (pwrCurve[_idx].offset * 100) + ( (s32)(rpm * 100) / pwrCurve[_idx].ratio);
  ign.delayCharge = (rotDur * procPwr) / 8000;
}

inline float getVoltage(void) {
  return (analogRead(A0)/1023.0)*10.2;
}

inline void calcCycleDur() {
  ign.lastZero = ign.zeroTime;
  ign.zeroTime = micros();
  rotDur = ign.zeroTime - ign.lastZero;  
}

inline void handleInput(void) {
  bool isEdge = false;
  if(!tmrDeadTime.isActive()) {
    ignIn = (u8)digitalRead(PIN_IGN_SENS);
  }
  // waiting for start of ignition cycle - rising or falling edge
  if(!lastIgnIn && ignIn) {
    tmr_L.set(ign.delaySpark, &spark_L);
    isEdge = true;
    charge_L();
  } else if(lastIgnIn && !ignIn) {
    tmr_R.set(ign.delaySpark, &spark_R);
    isEdge = true;
    charge_R();
  }
  if(isEdge) {
    tmrDeadTime.set(DEAD_TIME_EDGE);
    calcCycleDur();
    calcRpm();
    calcIgnData();
    tmrIdle.reset();
    lastIgnIn = ignIn;
  }
}

void setup() {
  pinMode(PIN_IGN_SENS,  INPUT);
  spark_L(); // off
  spark_R(); // off
  pinMode(L_IGN_PIN_OUT, OUTPUT);
  pinMode(R_IGN_PIN_OUT, OUTPUT);
  pinMode(A0, INPUT); // voltage measurement
  delay(500);
  volt = getVoltage();
  String s = String(volt);
  s = "Jawa350 is the best! BAT: " + s + "V";
  WiFi.softAP((char*)&s[0], "", 4, 0);
  ignIn = (u8)digitalRead(PIN_IGN_SENS);
  lastIgnIn = ignIn;
  #if _DEBUG
    Serial.begin(115200);
  #endif
}

//          | < ---- counted from last half revDur -----------------> |
//          |                  <----- cycleDur ----->                 |
//          |                      |            |                     |
//      time ZERO    delay      nextSparkTime      nextChargeTime       again time ZERO
//          |                      | mosfet off | mosfet on           |
//----------|----------------------|------------|------------//-------|-----

void loop() {
  handleInput();
  tmr_L.compute();
  tmr_R.compute();
  tmrIdle.compute();
  tmrDeadTime.compute();

  #if _DEBUG
    if(!(debugCnt % 100000)) {
      Serial.printf("rotDur: %d \t rpm: %d\t angle: %d\t Delays: %d\t %d\n", 
                     rotDur, rpm, angle, ign.delaySpark, ign.delayCharge);
    }
    debugCnt++;
  #endif
}