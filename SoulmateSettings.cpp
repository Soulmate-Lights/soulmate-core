
// #ifndef BUILDER_LIBRARIES_SOULMATE_SETTINGS_H_
// #define BUILDER_LIBRARIES_SOULMATE_SETTINGS_H_

#include "./SoulmateSettings.h"

// using namespace SoulmateSettings;

// namespace SoulmateSettings {

  String SoulmateSettings::stringValue(bool value) {
    return value ? "true" : "false";
  }

  bool SoulmateSettings::startInWifiMode() {
    return readFile("/startInWifiMode.txt") == "true";
  }

  bool SoulmateSettings::shouldCycle() {
  #ifdef SOULMATE_DISABLE_CYCLE
    return false
  #endif
  #ifdef FORCE_CYCLE
        return true;
  #endif
    // Cycle patterns by default
    return readFile("/cycle.txt") != "false";
  }

  void SoulmateSettings::setShouldCycle(bool shouldCycle) {
    writeFile("/cycle.txt", stringValue(shouldCycle));
  }

  bool SoulmateSettings::shouldBeOn() {
    return readFile("/on.txt") == "on";
  }

  void SoulmateSettings::setShouldBeOn(bool shouldBeOn) {
    writeFile("/on.txt", stringValue(shouldBeOn));
  }

  String SoulmateSettings::readSavedName() {
    return readFile("/name.txt");
  }

  void SoulmateSettings::saveName(String name) {
    writeFile("/name.txt", name);
  }

  void SoulmateSettings::saveRoutine(int routine) {
    writeFile("/routine.txt", (String)routine);
  }

  int SoulmateSettings::savedRoutine() {
    return readFile("/routine.txt").toInt();
  }

  void SoulmateSettings::saveBrightness(int brightness) {
    writeFile("/brightness.txt", (String)brightness);
  }

  int SoulmateSettings::savedBrightness() {
    return readFile("/brightness.txt").toInt();
  }
// } // namespace SoulmateSettings

// #endif
