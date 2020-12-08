#define FIRMWARE_NAME "livingroom"
#define N_LEDS 1200
#define SOULMATE_MILLIAMPS 3000
#define LED_TYPE WS2812B
#define FADE_DURATION 10
#define LED_COLS 1200
#define LED_ROWS 1
#define USE_WS2812B true
#define USE_BLUETOOTH true

#include <Soulmate.h>

void pattern() {
}

void setup() {
  Soulmate.addRoutine("pattern", pattern);

  Soulmate.setup();
}

void loop() {
  Soulmate.loop();
}
