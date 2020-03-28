#ifndef BUILDER_LIBRARIES_SOULMATE_CONFIG_H_
#define BUILDER_LIBRARIES_SOULMATE_CONFIG_H_

#ifdef ESP32
  #include "soc/timer_group_reg.h"
  #include "soc/timer_group_struct.h"
static TaskHandle_t FastLEDshowTaskHandle = 0;
#endif

// STD patches for newer C++ compiler for the Teensy compiler
// that expects these things to exist.
#ifdef CORE_TEENSY
namespace std {
  void __throw_bad_function_call() {
  }
  void __throw_length_error(char const *) {
  }
  void __throw_out_of_range_fmt(char const *fmt, ...) {
  }
} // namespace std
#endif

// Copyright 2019 Soulmate Lighting, LLC
// FastLED configuration

#define FASTLED_INTERNAL // removes pragma marks

#include <FastLED.h>

// Soulmate configuration

#ifndef LED_COLS
  #define LED_COLS 13
#endif

#ifndef LED_ROWS
  #define LED_ROWS 13
#endif

#ifndef N_LEDS
  #define N_LEDS 37
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

#ifdef CORE_TEENSY
  #define SOULMATE_DATA_PIN 18
  #define SOULMATE_CLOCK_PIN 23
  #define SOULMATE_BUTTON_PIN 5
#endif

#ifdef ESP32
  #ifndef SOULMATE_DATA_PIN
    #define SOULMATE_DATA_PIN 18
  #endif
  #ifndef SOULMATE_CLOCK_PIN
    #define SOULMATE_CLOCK_PIN 23
  #endif
#endif

#ifdef ESP32
  #ifndef SOULMATE_BUTTON_PIN
    #define SOULMATE_BUTTON_PIN 5
  #endif
#endif

#ifndef BLUETOOTH_PIN
  #define BLUETOOTH_PIN 14
#endif

#ifndef BLUETOOTH_MODULE_TX_PIN
  #define BLUETOOTH_MODULE_TX_PIN 13
#endif

#ifndef BLUETOOTH_MODULE_RX_PIN
  #define BLUETOOTH_MODULE_RX_PIN 12
#endif

#ifndef CYCLE_LENGTH_IN_MS
  #define CYCLE_LENGTH_IN_MS 60000
#endif

#ifndef FADE_DURATION
  #define FADE_DURATION 2000
#endif

#ifndef BUTTON_ON_VALUE
  #define BUTTON_ON_VALUE HIGH
#endif

#endif // BUILDER_LIBRARIES_SOULMATE_CONFIG_H_
