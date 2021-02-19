#include "./SoulmateFiles.h"

#ifndef BUILDER_LIBRARIES_SOULMATE_SETTINGS_H_
#define BUILDER_LIBRARIES_SOULMATE_SETTINGS_H_

namespace SoulmateSettings {}

namespace SoulmateSettings {
  String stringValue(bool value);
  bool startInWifiMode();
  bool shouldCycle();
  void setShouldCycle(bool shouldCycle);
  bool shouldBeOn();
  void setShouldBeOn(bool shouldBeOn);
  String readSavedName();
  void saveName(String name);
  void saveRoutine(int routine);
  int savedRoutine();
  void saveBrightness(int brightness);
  int savedBrightness();
} // namespace SoulmateSettings

#endif
