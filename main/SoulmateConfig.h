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

// Chipset selection.
//
// SK9822 used to be the only clocked option, so an APA102 panel was driven by
// the SK9822 controller. At the end of a long chain the two are not
// interchangeable: FastLED's APA102Controller::endBoundary() sends 0xFF filler
// bytes after the last pixel, SK9822Controller::endBoundary() sends 0x00
// (chipsets.h:212 and :274 in the pinned 3.4.0).
//
// A real APA102 re-clocks the signal and delays it by one cycle per LED, so the
// tail of a long chain is still shifting data through when the end frame
// arrives — and 32 zero bits are exactly what a start frame looks like, so
// those LEDs can latch the filler as pixel data and then run misaligned.
// Wrong, changing colours in the last rows of a long panel is the symptom.
//
// Define USE_APA102 for genuine APA102/DotStar. Note the market labels these
// two interchangeably, so this is worth trying either way on a panel that
// misbehaves at the far end.
//
// An explicit LED_TYPE from the app now wins, rather than being silently
// overwritten with SK9822 below.
// USE_SK9822 is listed explicitly even though it is also the fallback, so the
// firmware builder can emit exactly one define per chipset rather than relying
// on "emit nothing and hope". That is what went wrong before: the configurator
// offered APA102, emitted nothing for it, and got SK9822 by fallthrough.
#ifndef LED_TYPE
  #ifdef USE_WS2812B
    #define LED_TYPE WS2812B
  #elif defined(USE_APA102)
    #define LED_TYPE APA102
  #elif defined(USE_SK9822)
    #define LED_TYPE SK9822
  #else
    #define LED_TYPE SK9822
  #endif
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
// (fastspi_bitbang.h:123-124 switches formula on `SPI_SPEED > 10`), which is why
// asking for 12 MHz is faster than asking for 24. Values of 6 and below behave
// monotonically, so tune downward from there.
#ifndef SOULMATE_LED_DATA_RATE_MHZ
  #define SOULMATE_LED_DATA_RATE_MHZ 24
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
