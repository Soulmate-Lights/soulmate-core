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

// SPI clock for the clocked chipsets. This was not reachable at all: the
// addLeds() call passed four template arguments, which selects the FastLED
// overload that takes no data rate, so the controller's own default applied —
// DATA_RATE_MHZ(24) for SK9822.
//
// On a 240 MHz ESP32 that is not 24 MHz. FastLED 3.4.0 has no ESP32
// hardware-SPI backend, so this is bit-banged: DATA_RATE_MHZ(24) gives
// SPI_SPEED = 10, which costs ~25 delay cycles per bit and lands near 7 MHz.
// Long chains are where that bites, because the tail of the panel sees the most
// degraded clock edges.
//
// Measured cost at 1100 LEDs (~36,400 bits including start and end frames),
// against the 16 ms budget at SOULMATE_FPS 60:
//
//   24 (default)  ~7.0 MHz   ~5.3 ms
//   12            ~10 MHz    ~3.6 ms   <-- yes, faster than 24; see below
//    6            ~5.5 MHz   ~6.7 ms
//    4            ~3.7 MHz   ~9.7 ms
//    2            ~1.9 MHz  ~18.8 ms   does not fit 60fps at this size
//
// The delay macro is discontinuous at SPI_SPEED == 10
// (fastspi_bitbang.h:123-124 switches formula on `SPI_SPEED > 10`), and the two
// formulas do not meet. So the ladder is NOT monotonic in this number, and the
// direction is the opposite of the intuitive one. Measured at 1100 LEDs
// (~36,400 bits including start and end frames), F_CPU 240 MHz:
//
//   requested   divider   actual      frame
//      24         10      ~6.9 MHz    5.3 ms   <-- default
//      23         10      ~6.9 MHz    5.3 ms
//      22         10      ~6.9 MHz    5.3 ms
//      21         11     ~14.1 MHz    2.6 ms   <-- the cliff
//      20         12     ~13.3 MHz    2.7 ms
//      16         15     ~12.6 MHz    2.9 ms
//      13         18     ~10.9 MHz    3.3 ms
//      12         20     ~10.0 MHz    3.6 ms
//      10         24      ~8.6 MHz    4.2 ms
//       8         30      ~7.1 MHz    5.1 ms
//       6         40      ~5.5 MHz    6.7 ms
//       4         60      ~3.7 MHz    9.7 ms
//       2        120      ~1.9 MHz   18.8 ms   does not fit 60fps at this size
//
// Read that carefully before reaching for a "slower" value. The 24 default is
// already close to the slowest setting available above 6, because it sits on the
// far side of the cliff. **To go slower, go to 6 or below.** 12 is not a slower
// setting; it is a 45% faster one.
#ifndef SOULMATE_LED_DATA_RATE_MHZ
  #define SOULMATE_LED_DATA_RATE_MHZ 24
#endif

// 13..21 all land between ~11 and ~14 MHz — up to twice the default — while
// reading as a reduction. There is no reason to want a value in there, and
// picking one by mistake makes a marginal chain worse in the exact situation
// where you were trying to make it better. Fail the build instead.
#if SOULMATE_LED_DATA_RATE_MHZ >= 13 && SOULMATE_LED_DATA_RATE_MHZ <= 21
  #error "SOULMATE_LED_DATA_RATE_MHZ between 13 and 21 is FASTER than the 24 default, not slower: FastLED's bit-bang delay formula changes shape at divider 10 (fastspi_bitbang.h:123). Use 24 for the current ~6.9MHz, 6/4/2 to go slower, or 12/10/8 if you actually want faster. See the table above."
#endif

// FastLED temporal dithering, on by default in FastLED whenever brightness is
// below 255. It fakes extra low-end bits by modulating pixels frame to frame,
// which depends on a steady frame rate — and on a panel this size the frame
// rate is not steady. Set to 0 if the panel shimmers at low brightness.
#ifndef SOULMATE_DITHER
  #define SOULMATE_DITHER 1
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

// Same, for brightness. Shorter than the routine debounce: a slider drag
// settles in well under a second, and there's no reason to risk losing the
// value to a power cut for ten.
#ifndef BRIGHTNESS_SAVE_DEBOUNCE_MS
  #define BRIGHTNESS_SAVE_DEBOUNCE_MS 3000
#endif

#ifndef FADE_DURATION
  #define FADE_DURATION 2000
#endif

#ifndef BUTTON_ON_VALUE
  #define BUTTON_ON_VALUE LOW
#endif

#endif // SOULMATE_CONFIG_H_
