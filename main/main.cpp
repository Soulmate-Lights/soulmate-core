#define FIRMWARE_NAME "soulmate"
#define SOULMATE_MILLIAMPS 3000
#define FADE_DURATION 10
#define LED_ROWS 8
#define LED_COLS 8
#define N_LEDS (LED_ROWS * LED_COLS)
#define SOULMATE_DATA_PIN 32
#define SOULMATE_CLOCK_PIN 26
#define SOULMATE_BUTTON_PIN 39
#define SOULMATE_SERPENTINE true
#define USE_WS2812B true

#include "Soulmate.h"

void pattern() { // DO NOT MERGE!!!
  int nSamples;
  int16_t *samples = audioI2SSamples(&nSamples);
  if (samples == NULL) {
    return;
  }

  CRGB *led;
  int16_t sample;
  for (int i = 0; i < N_LEDS; i++) {
    led = &Soulmate.leds[i];
    for (int j = 0; j < 3; j++) {
      sample = samples[(3*i+j) % nSamples];
      if (sample < 0) {
        sample = -sample;
      }
      
      sample >>= 1;
      if (sample > 255) {
        sample = 255;
      }
      
      led->raw[j] = sample;
    }
  }
}

void setup() {
  Soulmate.addRoutine("Pattern", pattern);

  Soulmate.setup();
}

void loop() {
  Soulmate.loop();
}
