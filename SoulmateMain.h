#pragma once

#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_

#define SOULMATE_VERSION "8.0.4"

// #define FASTLED_RMT_MAX_CHANNELS 1
// #define FASTLED_RMT_BUILTIN_DRIVER 1
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#define FASTLED_INTERNAL
// #include "./SoulmateBeatSin.h"
#include "./SoulmateCircadian.h"
#include "./SoulmateConfig.h"
#include "./SoulmateFiles.h"
#include "./SoulmateSettings.h"
#include "./components/ArduinoJson/ArduinoJson.h"
#include <Arduino.h>
#include <FastLED.h>
#include <functional>

#define MAX_NUMBER_OF_ROUTINES 20

void FastLEDshowTask(void *pvParameters);

class SoulmateLibrary {
public:
  // SoulmateLibrary();

  // Overridden by config later
  String name;
  int brightness;
  int hue;
  int saturation;
  bool on;
  bool cycle;
  // Cycle timer
  uint32_t lastCycle;

  // Button logic
  bool buttonOn;
  bool buttonIncreasingBrightness;
  uint32_t buttonPressStart;

  int currentRoutine;
  int previousRoutine;

  // We stop the lights when updating
  bool stopped;

  // Fade in / fade between patterns variables
  int startingFrames;
  int newBrightness;
  int32_t fadeStart;
  bool faded;
  int routineCount;

  void (*routines[MAX_NUMBER_OF_ROUTINES])();
  String routineNames[MAX_NUMBER_OF_ROUTINES];

  // 3 arrays of N_CELLS used for blending
  CRGB leds[N_CELLS];
  CRGB previousLeds[N_CELLS];
  CRGB nextLeds[N_CELLS];

  String ip();
  void updateWifiClients();
  void WifiLoop();
  void WifiSetup();
  bool wifiConnected();
  void disconnectWiFi();
  void reconnect();
  bool isStreaming();
  void connectTo(const char *ssid, const char *pass);
  void lightPercentage();
  void BluetoothSetup();
  void BluetoothLoop();
  void StartBluetooth();
  void StopBluetooth();

  String status(bool showLANIP = true);

  void stop();
  bool isStopped();
  void lightPercentage(float percentage);
  void setup();
  void nextRoutine();
  void adjustBrightness();

  void playCurrentRoutine();
  void factoryReset();
  void reverseLeds();
  void fastLedShow();
  void showPixels();

  void adjustFromButton();
  void loop();
  void addRoutine(String routineName, void (*routine)());
  void setPixel(int index, CRGB color);
  void chooseRoutine(int i);
  void setBrightness(int b);
  void turnOff();
  void turnOn();
  void toggleOnOff();
  float CurrentBrightnessAsFloat();
  void setName(String n);

  std::function<void(const JsonObject &)> _jsonCallback;

  void onJSON(std::function<void(const JsonObject &)> callback);

  void consumeJson(const JsonObject &root);
};

// Define the singleton instance we use
extern SoulmateLibrary Soulmate;

#endif // BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_
