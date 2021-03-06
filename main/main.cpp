#define FIRMWARE_NAME "soulmate"
#define N_LEDS 196
#define SOULMATE_MILLIAMPS 4000
#define LED_TYPE SK9822
// #define FADE_DURATION 10
#define LED_COLS 14
#define LED_ROWS 14
#define SOULMATE_DATA_PIN 32
#define SOULMATE_CLOCK_PIN 26
#define SOULMATE_BUTTON_PIN 39
#define SOULMATE_SERPENTINE true
// #define USE_WS2812B true

// NOCHECKIN
#define USE_MICROPHONE

#include "Soulmate.h"

void pattern() {
  for (int i = 0; i < N_LEDS; i++) {
    Soulmate.leds[i] = CRGB::Red;
  }
}

void setup() {
  Soulmate.addRoutine("Pattern", pattern);

  Soulmate.setup();
}

void loop() {
  Soulmate.loop();
}
