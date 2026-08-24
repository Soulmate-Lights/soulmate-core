// Copyright 2019 Soulmate Lighting, LLC

#ifndef SOULMATE_CONFIG_H_
#define SOULMATE_CONFIG_H_

#include <ArduinoJson.h>
#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"
static TaskHandle_t FastLEDshowTaskHandle = 0;

#ifndef LED_COLS
  #define LED_COLS 13
#endif

#ifndef LED_ROWS
  #define LED_ROWS 1
#endif

#ifndef SOULMATE_COLOR_ORDER
  #define SOULMATE_COLOR_ORDER BGR
#endif

#ifndef N_LEDS
  #define N_LEDS LED_COLS * LED_ROWS
#endif

#ifndef SOULMATE_SERPENTINE
  #define SOULMATE_SERPENTINE true
#endif

// Upside down
#ifndef SOULMATE_REVERSE
  #define SOULMATE_REVERSE false
#endif

// Left to right flip
#ifndef SOULMATE_MIRROR
  #define SOULMATE_MIRROR false
#endif

#ifndef SOULMATE_MILLIAMPS
  #define SOULMATE_MILLIAMPS 600
#endif

#ifndef N_CELLS
  #ifdef LED_COLS
    #ifdef LED_ROWS
      #define N_CELLS (LED_COLS * LED_ROWS)
    #endif
  #else
    #define N_CELLS N_LEDS
  #endif
#endif

#ifdef USE_WS2812B
  #define LED_TYPE WS2812B
#else
  #define LED_TYPE SK9822
#endif

#ifndef SOULMATE_DATA_PIN
  #define SOULMATE_DATA_PIN 18
#endif

#ifndef SOULMATE_CLOCK_PIN
  #define SOULMATE_CLOCK_PIN 23
#endif

#ifndef CYCLE_LENGTH_IN_MS
  #define CYCLE_LENGTH_IN_MS 60000
#endif

// Target frame rate for the render task. The scheduler tick rate caps this:
// see the static_assert on kFrameTicks in SoulmateMain.h.
//
// This used to be silently wrong. The code asked for 60fps via
// EVERY_N_MILLISECONDS(1000/60) while CONFIG_FREERTOS_HZ was 100, giving a 10ms
// tick — and a 16ms request sampled on a 10ms grid fires every 20ms, so the
// panel actually ran at 50fps. sdkconfig now sets CONFIG_FREERTOS_HZ=1000
// (arduino-esp32's own default), making a 1ms tick.
//
// Note the delivered rate is 62.5fps, not exactly 60: the period is
// (1000/60)/1ms = 16 ticks after integer division, so 16ms. That's the closest
// the 1ms tick can get without going under; 17 ticks would give 58.8fps.
#ifndef SOULMATE_FPS
  #define SOULMATE_FPS 60
#endif

// How long the current routine has to hold still before it's written to NVS.
// Long enough that cycling through patterns on the button, or an app dragging
// through a list, collapses into one write.
#ifndef ROUTINE_SAVE_DEBOUNCE_MS
  #define ROUTINE_SAVE_DEBOUNCE_MS 10000
#endif

#ifndef FADE_DURATION
  #define FADE_DURATION 2000
#endif

#ifndef BUTTON_ON_VALUE
  #define BUTTON_ON_VALUE LOW
#endif

#endif // SOULMATE_CONFIG_H_
