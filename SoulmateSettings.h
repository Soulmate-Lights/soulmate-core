#include "./SoulmateFiles.h"

#ifndef BUILDER_LIBRARIES_SOULMATE_SETTINGS_H_
  #define BUILDER_LIBRARIES_SOULMATE_SETTINGS_H_

namespace SoulmateSettings {

  String stringValue(bool value) {
    return value ? "true" : "false";
  }

  bool startInWifiMode() {
    return readFile("/startInWifiMode.txt") == "true";
  }

  bool shouldCycle() {
  #ifdef SOULMATE_DISABLE_CYCLE
    return false
  #endif
  #ifdef FORCE_CYCLE
        return true;
  #endif
    return readFile("/cycle.txt") == "true";
  }

  void setShouldCycle(bool shouldCycle) {
    writeFile("/cycle.txt", stringValue(shouldCycle));
  }

  bool shouldBeOn() {
    return readFile("/on.txt") == "on";
  }

  void setShouldBeOn(bool shouldBeOn) {
    writeFile("/on.txt", stringValue(shouldBeOn));
  }

  String readSavedName() {
    return readFile("/name.txt");
  }

  void saveName(String name) {
    writeFile("/name.txt", name);
  }

  void saveRoutine(int routine) {
    writeFile("/routine.txt", (String)routine);
  }

  int savedRoutine() {
    return readFile("/routine.txt").toInt();
  }

  void saveBrightness(int brightness) {
    writeFile("/brightness.txt", (String)brightness);
  }

  int savedBrightness() {
    return readFile("/brightness.txt").toInt();
  }
} // namespace SoulmateSettings

#endif
