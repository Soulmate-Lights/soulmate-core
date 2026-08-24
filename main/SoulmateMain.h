// Copyright (2018) Soulmate Lighting, LLC

#ifndef SOULMATE_SOULMATEMAIN_H_
#define SOULMATE_SOULMATEMAIN_H_

#define SOULMATE_VERSION "9.1.0"

// #define FASTLED_RMT_MAX_CHANNELS 1
// #define FASTLED_RMT_BUILTIN_DRIVER 1
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#define FASTLED_INTERNAL

#include "SoulmateBeatSin.h"
#include "SoulmateCircadian.h"
#include "SoulmateConfig.h"
#include "SoulmateFiles.h"
#include "SoulmateSettings.h"
#include "SoulmateMatrix.h"
#include <ArduinoJson.h>
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <FastLED.h>
#include <functional>

#define MAX_NUMBER_OF_ROUTINES 20
void FastLEDshowTask(void *pvParameters);

// The render task's period in scheduler ticks. A tick is
// 1000/CONFIG_FREERTOS_HZ ms, so the tick rate is a hard ceiling on the frame
// rate. Computed here rather than in SoulmateConfig.h because
// portTICK_PERIOD_MS contains a cast, which the preprocessor can't evaluate in
// an #if but C++ can evaluate in a constant expression.
static constexpr TickType_t kFrameTicks =
    (1000 / SOULMATE_FPS) / portTICK_PERIOD_MS;

static_assert(kFrameTicks >= 1,
              "SOULMATE_FPS is faster than CONFIG_FREERTOS_HZ can schedule. "
              "Raise CONFIG_FREERTOS_HZ or lower SOULMATE_FPS.");

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

  // Routines - we use a max number to initalize this array
  int routineCount = 0;
  void (*routines[MAX_NUMBER_OF_ROUTINES])();
  String routineNames[MAX_NUMBER_OF_ROUTINES];

  // The framebuffer FastLED clocks out. This one has to stay in internal DRAM:
  // FastLED's RMT driver refills its buffers from an ISR registered with
  // ESP_INTR_FLAG_IRAM, which runs with the flash cache disabled, and PSRAM is
  // unreachable from there.
  CRGB leds[N_CELLS];

  // Crossfade scratch. The outgoing and incoming patterns each need their own
  // persistent framebuffer, because plenty of patterns read back what they drew
  // last frame (fadeToBlackBy, blur, trails).
  //
  // Only showPixels() touches these, never an ISR, so they can live in PSRAM.
  // That takes the internal-DRAM cost from 9 bytes per LED down to 3, which is
  // what was making large panels hard to fit. Allocated on the first crossfade
  // rather than at boot, so a build that never fades never pays for them.
  CRGB *previousLeds = nullptr;
  CRGB *nextLeds = nullptr;
  bool fadeBuffersUnavailable = false;

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

    #ifdef SOULMATE_BUILD
      message["build"] = SOULMATE_BUILD;
    #endif

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
    message["reverse"] = SOULMATE_REVERSE;
    message["mirror"] = SOULMATE_MIRROR;
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

      // 2D
      if (LED_ROWS > 1 && LED_COLS > 1) {
        ledsToFill = ledsToFill - ledsToFill % LED_COLS;
      }

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
    FastLED.addLeds<WS2812B, SOULMATE_DATA_PIN, SOULMATE_COLOR_ORDER>(leds,
                                                                      N_CELLS);
#else
    FastLED.addLeds<LED_TYPE, SOULMATE_DATA_PIN, SOULMATE_CLOCK_PIN,
                    SOULMATE_COLOR_ORDER>(leds, N_CELLS);
#endif

#ifdef SOULMATE_BUTTON_PIN
    pinMode(SOULMATE_BUTTON_PIN, INPUT_PULLDOWN);
#endif

    // Priority 3, matching AsyncTCP's own task (AsyncTCP.cpp:221), which also
    // runs on core 0 via CONFIG_ASYNC_TCP_RUNNING_CORE=0.
    //
    // This was priority 10. FreeRTOS is strictly priority-preemptive, so at 10
    // the render task pre-empted the entire network stack every time it woke —
    // which is why websockets and pixel streaming went sluggish under heavy
    // patterns. Any priority above 3 has the same problem; only equal priority
    // lets the two time-slice instead of one starving the other.
    xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 3,
                            &FastLEDshowTaskHandle, 0);

    WifiSetup();
#ifndef SKIP_BLUETOOTH
    BluetoothSetup();
#endif

    Serial.println(status(true));
  }

  // persist = whether this change reflects something the user actually asked
  // for. Auto-cycling passes false: it happens every CYCLE_LENGTH_IN_MS forever,
  // and there's nothing worth restoring about "whichever pattern the timer
  // happened to land on when the power went out".
  void nextRoutine(bool persist = true) {
    if (currentRoutine < 0) return;
    int i = currentRoutine + 1;
    if (i == routineCount)
      i = 0;
    chooseRoutine(i, persist);
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
    if (currentRoutine == -2) {
      // Don't do anything we're good
    } else if (currentRoutine == -1) {
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

  void reverseLeds() {
    // Flip vertical
    if (SOULMATE_REVERSE) {
      for (uint16_t x = 0; x < LED_COLS; x++) {
        for (uint16_t y = 0; y < LED_ROWS / 2; y++) {
          uint16_t one = XY(x, LED_ROWS - 1 - y);
          uint16_t two = XY(x, y);
          CRGB temp = leds[one];
          leds[one] = leds[two];
          leds[two] = temp;
        }
      }
    }

    // // Flip horizontal
    if (SOULMATE_MIRROR) {
      for (uint16_t y = 0; y < LED_ROWS; y++) {
        for (uint16_t x = 0; x < LED_COLS / 2; x++) {
          uint16_t one = XY(LED_COLS - 1 - x, y);
          uint16_t two = XY(x, y);
          CRGB temp = leds[one];
          leds[one] = leds[two];
          leds[two] = temp;
        }
      }
    }
  }

  // Pushes the framebuffer out.
  //
  // The frame clock used to live in here as EVERY_N_MILLISECONDS(1000/60),
  // which meant the caller ran its whole body — pattern render, blend, buffer
  // copies — on every pass and then discarded the result unless the gate
  // happened to be open. showPixels() was called every 10ms against a 16ms
  // gate, so roughly half of all render work was thrown away.
  //
  // The clock is now the render task's own period (kFrameTicks), so showPixels()
  // is only entered when the frame it produces will actually be shown.
  void fastLedShow() {
    reverseLeds();
    FastLED.show();
    reverseLeds();
  }

  // Render cost in microseconds. Nothing measured what a pattern actually costs
  // before this, so there was no way to tell a pattern that fits the frame
  // budget from one that doesn't.
  //
  // Deliberately NOT in status(). That builds a StaticJsonBuffer<2048> on the
  // caller's stack, and consumeJson() can reach it from the NimBLE host task,
  // whose stack is CONFIG_BT_NIMBLE_TASK_STACK_SIZE=4096 — so the buffer is
  // already half that task's stack. It's also close enough to full that
  // ArduinoJson 5 starts silently dropping trailing keys with a full gallery of
  // long routine names. Adding to it was the wrong place; this gets served on
  // its own with a buffer sized for it.
  uint32_t lastFrameUs = 0;
  uint32_t peakFrameUs = 0;

  String frameStats() {
    StaticJsonBuffer<192> jsonBuffer;
    JsonObject &message = jsonBuffer.createObject();
    message["fps"] = SOULMATE_FPS;
    message["frameBudgetUs"] = 1000000 / SOULMATE_FPS;
    message["frameUs"] = lastFrameUs;
    message["peakFrameUs"] = peakFrameUs;
    String out;
    message.printTo(out);
    return out;
  }

  // Allocates the two crossfade buffers, preferring PSRAM and falling back to
  // internal DRAM so boards without PSRAM keep the behaviour they have today.
  // Returns false if neither worked, in which case the caller should hard-cut
  // instead of fading.
  //
  // Only tried once. If there's no room now there won't be room next frame
  // either, and retrying at 60fps would just burn cycles.
  bool ensureFadeBuffers() {
    if (previousLeds && nextLeds)
      return true;
    if (fadeBuffersUnavailable)
      return false;

    const size_t bytes = sizeof(CRGB) * N_CELLS;
    bool inPsram = true;

    previousLeds = allocFadeBuffer(bytes, &inPsram);
    nextLeds = allocFadeBuffer(bytes, &inPsram);

    if (!previousLeds || !nextLeds) {
      heap_caps_free(previousLeds);
      heap_caps_free(nextLeds);
      previousLeds = nullptr;
      nextLeds = nullptr;
      fadeBuffersUnavailable = true;
      Serial.println(F("[Soulmate] No room for crossfade buffers. Pattern "
                       "transitions will cut instead of fading."));
      return false;
    }

    fill_solid(previousLeds, N_CELLS, CRGB::Black);
    fill_solid(nextLeds, N_CELLS, CRGB::Black);

    Serial.printf("[Soulmate] Crossfade buffers: 2 x %u bytes in %s. "
                  "Free internal: %u, largest block: %u\n",
                  static_cast<unsigned>(bytes), inPsram ? "PSRAM" : "DRAM",
                  static_cast<unsigned>(
                      heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
                  static_cast<unsigned>(
                      heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
    return true;
  }

  // Prefers PSRAM. Clears *inPsram if either buffer had to fall back to
  // internal DRAM, so the caller can report where they actually landed.
  static CRGB *allocFadeBuffer(size_t bytes, bool *inPsram) {
    void *buffer = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (!buffer) {
      buffer = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
      *inPsram = false;
    }
    return static_cast<CRGB *>(buffer);
  }

  void showPixels() {
    if (isStopped())
      return;

    if (isStreaming()) {
      fastLedShow();
      return;
    }

    int64_t frameStart = esp_timer_get_time();

    spi_flash_op_lock();

    // This function is pinned to a core.
    // If you put anything with SPIFFS in here, it may crash
    // with Stack canary watchpoint triggered (FastLEDshowTask)
    // or something similar. Keep it to pixels.

    uint32_t fadeMs = millis() - fadeStart;

    if (fadeMs < FADE_DURATION && ensureFadeBuffers()) {
      uint8_t percentage =
          static_cast<float>(fadeMs) / static_cast<float>(FADE_DURATION) * 255;
      const size_t size = sizeof(CRGB) * N_CELLS;

      // First frame of this transition: the incoming pattern should start from
      // black. This used to be cleared on every non-fading frame instead, which
      // meant writing a whole framebuffer to black ~60 times a second to set up
      // a transition that mostly wasn't happening.
      if (!faded)
        fill_solid(nextLeds, N_CELLS, CRGB::Black);

      // Grab the last frame from the previous pattern and run with it
      if (faded)
        memcpy(leds, previousLeds, size);
      if (previousRoutine >= 0)
        routines[previousRoutine]();
      memcpy(previousLeds, leds, size);
      // Put the next pattern's arrays into leds and run with it
      memcpy(leds, nextLeds, size);
      playCurrentRoutine();
      memcpy(nextLeds, leds, size);
      // Blend the two together
      for (int i = 0; i < N_CELLS; i++) {
        CRGB pixel = blend(CRGB::Black, previousLeds[i], 255 - percentage);
        pixel = blend(pixel, leds[i], percentage);
        leds[i] = pixel;
      }
      fastLedShow();
      faded = true;
    } else {
      // No clearing of previousLeds here: the crossfade path always overwrites
      // it from leds before the blend reads it, so zeroing it was dead work.
      playCurrentRoutine();
      fastLedShow();
      faded = false;
    }

    spi_flash_op_unlock();

    lastFrameUs = static_cast<uint32_t>(esp_timer_get_time() - frameStart);
    if (lastFrameUs > peakFrameUs)
      peakFrameUs = lastFrameUs;
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
    if (isStopped()) {
      return;
    }

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
      nextRoutine(false); // timer-driven, not user intent: don't persist
      lastCycle = millis();
    }

    adjustBrightness();
    flushRoutineIfSettled();
    if (currentRoutine >= routineCount)
      chooseRoutine(0, false); // recovering from a bad index, not user intent

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

  void chooseRoutine(int i, bool persist = true) {
    if (i == currentRoutine)
      return;
    previousRoutine = currentRoutine;
    if (millis() - fadeStart > FADE_DURATION)
      fadeStart = millis();
    currentRoutine = i;
    if (persist)
      routineNeedsSaving = true;
  }

  // Routine persistence is deferred rather than written inline.
  //
  // saveRoutine() is an NVS write, and a flash write on the ESP32 disables the
  // cache on *both* cores while it runs — so writing from chooseRoutine() put a
  // stall directly in the path of whatever the render task was doing. With
  // auto-cycle on that fired every 60 seconds, forever, which is a visible hitch
  // plus continuous flash wear for a value nobody reads until the next boot.
  //
  // Now: mark dirty, and flush from loop() once the value has settled.
  // INT32_MIN, not -1: -1 and -2 are real values of currentRoutine (solid
  // colour and streaming), so they can't double as "nothing pending".
  bool routineNeedsSaving = false;
  int pendingRoutine = INT32_MIN;
  uint32_t routineDirtiedAt = 0;

  // Called from loop(), which also means persistence now happens on one task
  // instead of from whichever of AsyncTCP / NimBLE / serial handled the request.
  void flushRoutineIfSettled() {
    if (!routineNeedsSaving)
      return;

    // Restart the timer while the routine is still moving, so cycling through
    // patterns on the button writes once at the end rather than once each.
    if (currentRoutine != pendingRoutine) {
      pendingRoutine = currentRoutine;
      routineDirtiedAt = millis();
      return;
    }

    if (millis() - routineDirtiedAt < ROUTINE_SAVE_DEBOUNCE_MS)
      return;

    routineNeedsSaving = false;
    if (currentRoutine >= 0)
      SoulmateSettings::saveRoutine(currentRoutine);
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
      SoulmateSettings::saveBrightness(brightness);
      setBrightness(brightness);
    }

    if (root.containsKey("routine")) {
      chooseRoutine(static_cast<int>(root["routine"]));
    }

    if (root.containsKey("SSID") && root.containsKey("WIFIPASS")) {
      Serial.println("Saving SSID:");
      const char *ssid = root["SSID"].as<char *>();
      Serial.println(ssid);
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

// ESP32 dual-core task, pinned to a core.
//
// vTaskDelayUntil rather than vTaskDelay: vTaskDelay sleeps for the period
// *after* the work finishes, so the real frame interval was render time + 10ms
// and drifted with how expensive the pattern was. Delaying until an absolute
// deadline gives a fixed period and absorbs render-time variation.
void FastLEDshowTask(void *pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    // Disable the Task watchdog checking for a second-core task!
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
    Soulmate.showPixels();

    // If we overran the frame — a flash write stalling both cores, an OTA, a
    // pattern that took too long — resync to now. Left alone, vTaskDelayUntil
    // would stop blocking and replay every missed frame back-to-back to catch
    // up, which reads as the animation lurching forwards.
    //
    // The vTaskDelay(1) matters: at priority 3 this task must yield or core 0's
    // idle task never runs, and CONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU0 is on.
    TickType_t now = xTaskGetTickCount();
    if (static_cast<TickType_t>(now - lastWake) > kFrameTicks) {
      lastWake = now;
      vTaskDelay(1);
    } else {
      vTaskDelayUntil(&lastWake, kFrameTicks);
    }
  }
}

#endif // SOULMATE_SOULMATEMAIN_H_
