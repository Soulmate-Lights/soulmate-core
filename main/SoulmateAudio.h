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

// All audio
esp_err_t audioDisable(void);

// I2S Audio
esp_err_t audioDisableI2S(void);
esp_err_t audioEnableI2S(audio_mode_t mode);
int audioI2SSamples(int16_t **samples);

#endif /* soulmate_audio_h */
