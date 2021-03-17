#ifndef SOULMATE_BEATSIN16_H_
#define SOULMATE_BEATSIN16_H_

#include <FastLED.h>

// like beatsin16, but it returns a float
float beatsin16float(float beats_per_minute, float lowest = 0,
                     float highest = 65535, uint32_t timebase = 0,
                     uint16_t phase_offset = 0) {
  float multiplier = 1000.0;
  float result =
      (float)beatsin16(beats_per_minute * multiplier, lowest * multiplier,
                       highest * multiplier, timebase, phase_offset);
  return result / multiplier;
}

#endif // SOULMATE_BEATSIN16_H_
