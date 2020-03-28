#ifndef BUILDER_LIBRARIES_SOULMATE_BEATSIN16_H_
#define BUILDER_LIBRARIES_SOULMATE_BEATSIN16_H_

#include <FastLED.h>

float multiplier = 100.0;

float beatsin16float(uint16_t bpm, uint16_t min, uint16_t max) {
  return (float)beatsin16(bpm, (float)min * multiplier,
                          (float)max * multiplier) /
         multiplier;
}

#endif // BUILDER_LIBRARIES_SOULMATE_BEATSIN16_H_
