#define FIRMWARE_NAME "soulmate"
#define N_LEDS 64
#define SOULMATE_MILLIAMPS 3000
#define LED_TYPE WS2812B
#define FADE_DURATION 10
#define LED_COLS 8
#define LED_ROWS 8
#define SOULMATE_DATA_PIN 27
#define SOULMATE_CLOCK_PIN 29
#define USE_WS2812B true

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
