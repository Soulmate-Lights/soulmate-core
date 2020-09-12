// Copyright (2018) Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_

#define SOULMATE_VERSION "7.1.6"

// #define FASTLED_RMT_MAX_CHANNELS 1
// #define FASTLED_RMT_BUILTIN_DRIVER 1
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#define FASTLED_INTERNAL

#include "./SoulmateBeatSin.h"
#include "./SoulmateCircadian.h"
#include "./SoulmateConfig.h"
#include "./SoulmateFiles.h"
#include "./SoulmateOTA.h"
#include "./SoulmateSettings.h"
#include "./components/ArduinoJson/ArduinoJson.h"
#include <Arduino.h>
#include <FastLED.h>
#include <functional>

#define MAX_NUMBER_OF_ROUTINES 20
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
  bool cycle = true;

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

    // These paraemters are for flash config
    message["rows"] = LED_ROWS;
    message["cols"] = LED_COLS;
    // This might want to say APA102?
    #ifdef USE_WS2812B
      message["ledType"] = "WS2812B";
    #else
      message["ledType"] = "SK9822";
    #endif
    message["serpentine"] = SOULMATE_SERPENTINE;
    message["milliamps"] = SOULMATE_MILLIAMPS;
    message["data"] = SOULMATE_DATA_PIN;
    message["clock"] = SOULMATE_CLOCK_PIN;
    // End app config

    #ifdef SOULMATE_BUTTON_PIN
      message["button"] = SOULMATE_BUTTON_PIN;
    #endif

    uint64_t chipid = ESP.getEfuseMac();
    message["chipId"] = (uint16_t)(chipid >> 32);

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
    if (percentage < 0.9) {
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

    SPIFFS.begin(true);

    // if (readFile("/start-off") == "true") {
    //   on = false;
    //   writeFile("/start-off", "false");
    // }

    Circadian::setup();

    cycle = SoulmateSettings::shouldCycle();

    FastLED.setBrightness(0);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, SOULMATE_MILLIAMPS);

    // Restore lamp name
    name = SoulmateSettings::readSavedName();

// Set up FastLED
#ifdef USE_WS2812B
    FastLED.addLeds<WS2812B, SOULMATE_DATA_PIN, SOULMATE_COLOR_ORDER>(leds,
                                                                      N_CELLS);
#else
    FastLED.addLeds<LED_TYPE, SOULMATE_DATA_PIN, SOULMATE_CLOCK_PIN,
                    SOULMATE_COLOR_ORDER>(leds, N_CELLS);
#endif

#ifdef SOULMATE_BUTTON_PIN
    pinMode(SOULMATE_BUTTON_PIN, INPUT_PULLDOWN);
#endif

#ifdef USE_WS2812B
    // Previously we used core 0 for APA102
    // TODO: Check the flashing on BigBoy to see if it's core-related
    // NOTE: Now Wifi runs on Core 1 (Jun 17) so this may all want to be on Core
    // 0.
    // TODO: Test on WS2812B lights
    xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 10,
                            &FastLEDshowTaskHandle, 1);
#else
    xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 10,
                            &FastLEDshowTaskHandle, 0);
#endif

    WifiSetup();
#ifndef SKIP_BLUETOOTH
    BluetoothSetup();
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

  void factoryReset() {
    deleteAllFiles();
    disconnectWiFi();
    ESP.restart();
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
      EVERY_N_MILLISECONDS(1000 / 60) {
        FastLED.show();
      }
      faded = true;
    } else {
      playCurrentRoutine();
      EVERY_N_MILLISECONDS(1000 / 60) {
        spi_flash_op_lock();
        FastLED.show();
        spi_flash_op_unlock();
      }
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

        // Factory reset! 10 seconds.
        if (buttonPressDuration > 10000) {
          factoryReset();
        }
      }

      // Finish pressing
      if (buttonOn && !buttonIsCurrentlyDown) {
        uint32_t buttonPressDuration = millis() - buttonPressStart;

        if (buttonPressDuration < 1000) {
          // If it's for less than a second, switch routine.
          nextRoutine();
        } else {
          // Otherwise, Set whether we're increasing or decreasing the
          // brightness.
          buttonIncreasingBrightness = !buttonIncreasingBrightness;
        }
      }
      buttonOn = buttonIsCurrentlyDown;
    }
#endif
  }

  String inputString = "";
  boolean stringComplete = false;

  void loop() {

    while (Serial.available()) {
      char inChar = (char)Serial.read();
      inputString += inChar;
      if (inChar == '\n') {
        stringComplete = true;
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(inputString);
        consumeJson(root);
        inputString = "";
      }
    }

    adjustFromButton();

    // // This is something we use for our internal Soulmate lights!
    // #ifdef AUTOMATIC_OTA_UPDATES
    //     EVERY_N_SECONDS(300) {
    //       if (wifiConnected()) {
    //         if (!on && FastLED.getBrightness() == 0 &&
    //             SoulmateOTA::shouldUpdate()) {
    //           writeFile("/start-off", on ? "false" : "true");
    //           stop();
    //           SoulmateOTA::update();
    //         }
    //       }
    //     }
    // #endif

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
  }

  void setBrightness(int b) {
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

    if (root.containsKey("status"))
      Serial.println(status(true));

    if (root.containsKey("restart"))
      ESP.restart();

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
      int brightness = static_cast<int>(root["brightness"]);
      setBrightness(brightness);
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
void SoulmateLibrary::StartBluetooth() {
}
void SoulmateLibrary::StopBluetooth() {
}
void SoulmateLibrary::BluetoothLoop() {
}
void SoulmateLibrary::BluetoothSetup() {
}
#endif

// ESP32 dual-core task, pinned to a core
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

#endif // BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_
