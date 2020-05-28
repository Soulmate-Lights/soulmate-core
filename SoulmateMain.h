// Copyright (2018) Soulmate Lighting, LLC


#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_

#define SOULMATE_VERSION "7.1.4"

#define FASTLED_INTERNAL

#include <Arduino.h>
#include <FastLED.h>
#include <functional>
#include "./SoulmateBeatSin.h"
#include "./SoulmateCircadian.h"
#include "./SoulmateConfig.h"
#include "./SoulmateFiles.h"
#include "./SoulmateSettings.h"
#include "./SoulmateOTA.h"
#include "./components/ArduinoJson/ArduinoJson.h"

#define MAX_NUMBER_OF_ROUTINES 25
void FastLEDshowTask(void *pvParameters);

class SoulmateLibrary {
 public:
  SoulmateLibrary() {
  }

  // Overridden by config later
  String name = F("New Soulmate");
  int brightness = 255;
  int hue = 0;
  int saturation = 0;
  bool on = true;
  bool cycle = false;

  // Cycle timer
  uint32_t lastCycle = millis();

  // Button logic
  bool buttonOn = false;
  bool buttonIncreasingBrightness = false;
  uint32_t buttonPressStart = 0;

  int currentRoutine = 0;
  int previousRoutine = -1;

  // We stop the lights when updating
  bool stopped = false;

  // Fade in / fade between patterns variables
  int startingFrames = 0;
  int newBrightness = 255;
  int32_t fadeStart;
  bool faded;

  // Used for saving
  bool brightnessDirty = false;
  bool routineDirty = false;

  // Routines - we use a max number to initalize this array
  int routineCount = 0;
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
  void connectTo(const char *ssid, const char *pass);

  void lightPercentage();

  void BluetoothSetup();
  void BluetoothLoop();
  void StartBluetooth();
  void StopBluetooth();

  String status(bool showLANIP = true) {
    StaticJsonBuffer<2048> jsonBuffer;
    JsonObject &message = jsonBuffer.createObject();
    JsonArray &routinesArray = message.createNestedArray("routines");
    routinesArray.copyFrom(routineNames, routineCount);

    message["routine"] = currentRoutine;
    message["name"] = name;
    message["on"] = on;
    message["brightness"] = brightness;
    message["version"] = SOULMATE_VERSION;
    message["cycle"] = cycle;
    message["circadian"] = Circadian::circadian;
    message["wakeTime"] = Circadian::wakeTime;
    message["sleepTime"] = Circadian::sleepTime;
    message["lanip"] = false;

    if (showLANIP)
      message["lanip"] = ip();

#ifdef ESP32
    uint64_t chipid = ESP.getEfuseMac();
    message["chipId"] = (uint16_t)(chipid >> 32);
#endif

#ifdef FIRMWARE_NAME
    message["firmwareName"] = FIRMWARE_NAME;
#endif

    String outputString;
    message.printTo(outputString);
    return outputString;
  }

  // Stopping pixels during update
  void stop() {
    stopped = true;
  }

  bool isStopped() {
    return stopped;
  }

  // Light up a percentage of the panel to represent update status
  void lightPercentage(float percentage) {
    fill_solid(leds, N_LEDS, CRGB::Black);
    if (percentage < 1) {
      uint16_t ledsToFill = (float)N_LEDS * percentage;
      if (N_LEDS > 100)
        ledsToFill = ledsToFill - ledsToFill % LED_COLS;
      fill_solid(leds, ledsToFill, CRGB::Green);
    } else {
      fill_solid(leds, N_LEDS, CRGB::Green);
    }
    FastLED.setBrightness(128);
    FastLED.show();
  }

  // Setup

  void setup() {
    // Clear a line for reading after flashing. Everything before this is 78400
    // baud boot nonsense from the ESP.
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Booting Soulmate v" + String(SOULMATE_VERSION));
    Serial.println("firmware=" + String(FIRMWARE_NAME) +
                   " version=" + String(SOULMATE_VERSION));

    lastCycle = millis();

#ifdef ESP32
    SPIFFS.begin(true);
#endif

    if (readFile("/start-off") == "true") {
      on = false;
      writeFile("/start-off", "false");
    }

    Circadian::setup();

    cycle = SoulmateSettings::shouldCycle();

    // Restore last brightness, and then we'll fade into it
    int savedBrightness = SoulmateSettings::savedBrightness();
    if (savedBrightness)
      brightness = savedBrightness;
    FastLED.setBrightness(0);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, SOULMATE_MILLIAMPS);

    // Restore lamp name
    name = SoulmateSettings::readSavedName();

    // Restore last routine
    int savedRoutine = SoulmateSettings::savedRoutine();
    if (savedRoutine && savedRoutine < routineCount)
      currentRoutine = savedRoutine;

// Set up FastLED
#ifdef USE_WS2812B
  #ifdef SOULMATE_COLOR_ORDER
    FastLED.addLeds<WS2812B, SOULMATE_DATA_PIN, SOULMATE_COLOR_ORDER>(leds,
                                                                      N_CELLS);
  #else
    FastLED.addLeds<WS2812B, SOULMATE_DATA_PIN, GRB>(leds, N_CELLS);
  #endif
#else
  #ifdef CORE_TEENSY
    FastLED.addLeds<LED_TYPE, SOULMATE_DATA_PIN, SOULMATE_CLOCK_PIN, BGR,
                    DATA_RATE_MHZ(1)>(leds, N_CELLS);
  #else
    FastLED.addLeds<LED_TYPE, SOULMATE_DATA_PIN, SOULMATE_CLOCK_PIN, BGR>(
        leds, N_CELLS);
  #endif
#endif

#ifdef ESP32 // These are the latest known good configuration depending on strip
             // type.
    xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 10,
                            &FastLEDshowTaskHandle, 1);
#endif

#ifdef ESP32
    WifiSetup();
  #ifndef SKIP_BLUETOOTH
    BluetoothSetup();
    // We used to use this for startup and printing to determine firmware
    // #else
    // Without BT it starts too fast to print
    // delay(1000);
  #endif
#endif

#ifdef SOULMATE_BUTTON_PIN
    pinMode(SOULMATE_BUTTON_PIN, INPUT_PULLDOWN);
#endif

  }

  void nextRoutine() {
    int i = currentRoutine + 1;
    if (i == routineCount)
      i = 0;
    chooseRoutine(i);
  }

  void adjustBrightness() {
    if (on) {
      // Fade in: adjust brightness slowly
      // Once we're done getting bright, set startingFrames to a too-high number
      // so we don't fade any more.
      if (startingFrames == brightness)
        startingFrames = 255;
      if (startingFrames < brightness)
        FastLED.setBrightness(startingFrames);

      EVERY_N_MILLISECONDS(5) {
        if (startingFrames < brightness) {
          EVERY_N_MILLISECONDS(20)
          startingFrames++;
        } else if (FastLED.getBrightness() > brightness) {
          FastLED.setBrightness(FastLED.getBrightness() - 1);
        } else if (FastLED.getBrightness() < brightness) {
          FastLED.setBrightness(FastLED.getBrightness() + 1);
        }
      }

    } else if (FastLED.getBrightness() > 0) {
      // Slowly fade to off
      EVERY_N_MILLISECONDS(5) {
        FastLED.setBrightness(FastLED.getBrightness() - 1);
      }
    }
  }

  void playCurrentRoutine() {
    if (currentRoutine == -1) {
      fill_solid(leds, N_LEDS, CHSV(hue, saturation, 255));
    } else {
      routines[currentRoutine]();
    }
  }

  void showPixels() {
    if (isStopped())
      return;

    // This function is pinned to a core.
    // If you put anything with SPIFFS in here, it may crash
    // with Stack canary watchpoint triggered (FastLEDshowTask)
    // or something similar. Keep it to pixels.

    uint32_t fadeMs = millis() - fadeStart;

    if (fadeMs < FADE_DURATION) {
      uint8_t percentage =
          static_cast<float>(fadeMs) / static_cast<float>(FADE_DURATION) * 255;
      int size = sizeof(leds);
      // Grab the last frame from the previous pattern and run with it
      if (faded)
        memcpy(&leds, &previousLeds, size);
      if (previousRoutine >= 0)
        routines[previousRoutine]();
      memcpy(&previousLeds, &leds, size);
      // Put the next pattern's arrays into leds and run with it
      memcpy(&leds, &nextLeds, size);
      playCurrentRoutine();
      memcpy(&nextLeds, &leds, size);
      // Blend the two together
      for (int i = 0; i < N_CELLS; i++) {
        CRGB pixel = blend(CRGB::Black, previousLeds[i], 255 - percentage);
        pixel = blend(pixel, leds[i], percentage);
        leds[i] = pixel;
      }
      FastLED.show();
      faded = true;
    } else {
      playCurrentRoutine();
      FastLED.show();
      fill_solid(previousLeds, N_LEDS, CRGB::Black);
      fill_solid(nextLeds, N_LEDS, CRGB::Black);
      faded = false;
    }
  }

  void adjustFromButton() {
    #ifdef SOULMATE_BUTTON_PIN
      EVERY_N_MILLISECONDS(10) {
        bool buttonSignal = digitalRead(SOULMATE_BUTTON_PIN);
        bool buttonIsCurrentlyDown = buttonSignal == BUTTON_ON_VALUE;

        if (!buttonOn && buttonIsCurrentlyDown) { // Start pressing
          buttonPressStart = millis();
          newBrightness = brightness;
        }

        // Keep pressing
        if (buttonIsCurrentlyDown && buttonOn) {
          uint32_t buttonPressDuration = millis() - buttonPressStart;
          if (buttonPressDuration > 500) {
            newBrightness = newBrightness + (buttonIncreasingBrightness ? 1 : -1);
            brightness = constrain(newBrightness, 0, 255);
          }
        }

        // Finish pressing
        if (buttonOn && !buttonIsCurrentlyDown) {
          uint32_t buttonPressDuration = millis() - buttonPressStart;

          if (buttonPressDuration < 1000) {
            // If it's for less than a second, switch routine.
            nextRoutine();
          } else {
            // Otherwise, Set whether we're increasing or decreasing the brightness.
            buttonIncreasingBrightness = !buttonIncreasingBrightness;
          }
        }
        buttonOn = buttonIsCurrentlyDown;
      }
    #endif
  }

  void loop() {
    adjustFromButton();

    // This is something we use for our internal Soulmate lights!
    #ifdef AUTOMATIC_OTA_UPDATES
      EVERY_N_SECONDS(300) {
        if (wifiConnected()) {
          if (!on && FastLED.getBrightness() == 0 && SoulmateOTA::shouldUpdate()) {
            writeFile("/start-off", on ? "false" : "true");
            stop();
            SoulmateOTA::update();
          }
        }
      }
    #endif

    EVERY_N_SECONDS(5) {
      switch (Circadian::checkTime()) {
      case Circadian::SHOULD_TURN_OFF:
        turnOff();
        break;
      case Circadian::SHOULD_TURN_ON:
        turnOn();
        break;
      }
    }

// Only try to save brightness and routine every 5 seconds. Saves bashing
// SPIFFS, and is much nicer on WS2812B strips because of the timing.
#ifndef SOULMATE_DISABLE_SAVING
    EVERY_N_SECONDS(5) {
      if (brightnessDirty) {
        SoulmateSettings::saveBrightness(brightness);
        brightnessDirty = false;
      }

      if (routineDirty) {
        SoulmateSettings::saveRoutine(currentRoutine);
        routineDirty = false;
      }
    }
#endif

    bool needsToCycle = millis() - lastCycle > CYCLE_LENGTH_IN_MS;
    if (cycle && needsToCycle) {
      nextRoutine();
      lastCycle = millis();
    }

    adjustBrightness();
    if (currentRoutine >= routineCount)
      chooseRoutine(0);

#ifndef SKIP_WIFI
    WifiLoop();
#endif

#ifndef SKIP_BLUETOOTH
    BluetoothLoop();
#endif

// ESP32 has multi-tasking, others have to display in the loop
#ifndef ESP32
    showPixels();
#endif
  }

  void addRoutine(String routineName, void (*routine)()) {
    routines[routineCount] = routine;
    routineNames[routineCount] = routineName;
    routineCount++;
  }

  void setPixel(int index, CRGB color) {
    leds[index] = color;
  }

  void chooseRoutine(int i) {
    if (i == currentRoutine)
      return;
    previousRoutine = currentRoutine;
    if (millis() - fadeStart > FADE_DURATION)
      fadeStart = millis();
    currentRoutine = i;
    routineDirty = true;
  }

  void setBrightness(int b) {
    brightnessDirty = true;
    if (startingFrames >= b)
      startingFrames = 255;
    if (b > 0 && !on)
      on = true;
    brightness = b;
  }

  void turnOff() {
    on = false;
  }

  void turnOn() {
    on = true;
  }

  void toggleOnOff() {
    on = !on;
  }

  float CurrentBrightnessAsFloat() {
    return static_cast<float>(FastLED.getBrightness()) / 255.0;
  }

  void setName(String n) {
    if (n.length() == 0)
      return;
    SoulmateSettings::saveName(n);
    name = n;
  }

  std::function<void(const JsonObject &)> _jsonCallback = NULL;

  void onJSON(std::function<void(const JsonObject &)> callback) {
    _jsonCallback = callback;
  }

  void consumeJson(const JsonObject &root) {
    if (_jsonCallback != NULL)
      _jsonCallback(root);

    if (root.containsKey("time")) {
      float receivedSeconds = root.get<float>("time");
      unsigned long currentSeconds = millis() / 1000;
      unsigned long startedSeconds = receivedSeconds - currentSeconds;
      Circadian::startTrackingTime(startedSeconds);
    }

    if (root.containsKey("updatePercentage")) {
      stop();
      float updatePercentage = static_cast<float>(root["updatePercentage"]);
      lightPercentage(updatePercentage);
    }

    if (root.containsKey("stop"))
      stop();
    if (root.containsKey("reconnect"))
      reconnect();
    if (root.containsKey("reset"))
      disconnectWiFi();

#ifdef ESP32
    if (root.containsKey("restart"))
      ESP.restart();
#endif

    if (root.containsKey("hue")) {
      currentRoutine = -1;
      int value = static_cast<int>(root["hue"]);
      float newHue = (float)value + 180;
      newHue = newHue / 360.0 * 255.0;
      hue = newHue;
    }

    if (root.containsKey("saturation")) {
      currentRoutine = -1;
      int value = static_cast<int>(root["saturation"]);
      saturation = (float)value / 100.0 * 255.0;
    }

    if (root.containsKey("cycle")) {
      cycle = static_cast<bool>(root["cycle"]);
      SoulmateSettings::setShouldCycle(cycle);
    }

    if (root.containsKey("circadian")) {
      bool circadian = static_cast<bool>(root["circadian"]);
      Circadian::saveCircadian(circadian);
    }

    if (root.containsKey("wakeTime") && root.containsKey("sleepTime")) {
      float wakeTime = static_cast<float>(root["wakeTime"]);
      float sleepTime = static_cast<float>(root["sleepTime"]);
      Circadian::setSleepTime(sleepTime);
      Circadian::setWakeTime(wakeTime);
    }

    if (root.containsKey("brightness")) {
      setBrightness(static_cast<int>(root["brightness"]));
    }

    if (root.containsKey("routine")) {
      chooseRoutine(static_cast<int>(root["routine"]));
    }

    if (root.containsKey("SSID") && root.containsKey("WIFIPASS")) {
      const char *ssid = root["SSID"].as<char *>();
      const char *pass = root["WIFIPASS"].as<char *>();
      connectTo(ssid, pass);
    }

    if (root.containsKey("on")) {
      root["on"] ? turnOn() : turnOff();
    }

    if (root.containsKey("Name")) {
      setName(root["Name"]);
    }

    if (root.containsKey("name")) {
      setName(root["name"]);
    }
  }
};

SoulmateLibrary Soulmate;

#ifdef SKIP_BLUETOOTH
void SoulmateLibrary::StartBluetooth() {}
void SoulmateLibrary::StopBluetooth() {}
void SoulmateLibrary::BluetoothLoop() {}
void SoulmateLibrary::BluetoothSetup() {}
#endif

// ESP32 dual-core task, pinned to a core
#ifdef ESP32
void FastLEDshowTask(void *pvParameters) {
  for (;;) {
    // Disable the Task watchdog checking for a second-core task!
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
    Soulmate.showPixels();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
#endif

#endif // BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_
