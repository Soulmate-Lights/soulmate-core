#ifndef soulmate_audio_h
#define soulmate_audio_h

#include <Arduino.h>
#include <Wire.h>
#include "driver/i2s.h"
#include "driver/timer.h"

typedef enum {
  MODE_MIC,
  MODE_SPK,
} audio_mode_t;

uint16_t audioAvailable(void);
esp_err_t InitI2SAudio(audio_mode_t mode);

#endif /* soulmate_audio_h */
