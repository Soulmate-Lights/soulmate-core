#define FASTLED_INTERRUPT_RETRY_COUNT 1

#define FIRMWARE_NAME "soulmate"
#define N_LEDS 256
#define SOULMATE_MILLIAMPS 4000
#define LED_TYPE WS2812B
// #define FADE_DURATION 10
#define LED_COLS 16
#define LED_ROWS 16
#define SOULMATE_DATA_PIN 32
#define SOULMATE_CLOCK_PIN 26
#define SOULMATE_BUTTON_PIN 39
#define SOULMATE_SERPENTINE true
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
