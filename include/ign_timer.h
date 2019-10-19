/*
 * ign_timer.h
 *
 *  Created on: Oct 8, 2019
 *      Author: Pavel Svatos
 */
#include <Arduino.h>
#include <c_types.h>

#ifndef IGN_TIMER_H_
#define IGN_TIMER_H_

typedef void (*callback_t)(void);

class ignTimer {
  private:
    u32 _start;
    u32 _delay;
    bool _active;
    callback_t _callback;
  public:
    ignTimer():
      _start(0),
      _delay(0), 
      _active(false),
      _callback(nullptr)
      {};
    ignTimer(u32 delay, callback_t callback):
      _start(micros()),
      _delay(delay), 
      _active(true),
      _callback(callback)
      {};
    void compute(void) {
      if (_active && ((micros() - _start) > _delay)) {        
        _active = false;
        if (_callback != nullptr) {
          (*_callback)();
        }
      }
    }
    void set(u32 delay, callback_t callback = nullptr) {
      _start    = micros();
      _delay    = delay;
      _callback = callback;
      _active   = true;
    }
    inline void reset(void) {
      _start  = micros();
      _active = true;
    }
    inline bool isActive(void) {
      return _active;
    }
};

#endif
