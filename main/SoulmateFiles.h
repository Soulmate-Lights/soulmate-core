// Copyright 2019 Soulmate Lighting, LLC

#ifndef SOULMATE_FILES_H_
#define SOULMATE_FILES_H_
#include "FS.h"

#include <SPIFFS.h>

#include <Preferences.h>
Preferences filesPreferences;

void writeFile(String name, String contents) {
  filesPreferences.begin("settings", false);
  filesPreferences.putString(name.c_str(), String(contents));
  filesPreferences.end();
}

String readFile(String name) {
  filesPreferences.begin("settings", false);
  String value = filesPreferences.getString(name.c_str(), "");
  filesPreferences.end();
  return value;
}

void deleteAllFiles() {
  filesPreferences.begin("settings", false);
  filesPreferences.clear();
  filesPreferences.end();
}

#endif // SOULMATE_FILES_H_
