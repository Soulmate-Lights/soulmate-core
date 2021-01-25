// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_FILES_H_
#define BUILDER_LIBRARIES_SOULMATE_FILES_H_
#include "FS.h"

#include <SPIFFS.h>

#include <Preferences.h>
Preferences filesPreferences;

// TODO(elliott): Change these to use "preferences" instead of SPIFFS

void writeFile(String name, String contents) {
  filesPreferences.begin("settings", false);
  filesPreferences.putString(name.c_str(), String(contents));
  filesPreferences.end();
// #if defined(ESP32) || defined(ESP8266)
//   File f = SPIFFS.open(name, "w");
//   f.print(contents);
//   f.flush();
//   f.close();
// #endif
}

String readFile(String name) {
  filesPreferences.begin("settings", false);
  String value = filesPreferences.getString(name.c_str(), "");
  filesPreferences.end();
  return value;

//   String ret;
// #if defined(ESP32) || defined(ESP8266)
  // File f = SPIFFS.open(name, "r");
  // if (!f)
  //   return "";
  // ret = f.readString();

  // String newString;
  // for (int i = 0; i < ret.length(); i++) {
  //   char inchar = ret[i];
  //   if ((' ' <= inchar) && (inchar <= '~')) newString += inchar;
  // }

  // ret = newString;

  // f.close();
// #endif
  // return ret;
}

void deleteAllFiles() {
  filesPreferences.begin("settings", false);
  filesPreferences.clear();
  filesPreferences.end();
  // SPIFFS.format();
}

#endif // BUILDER_LIBRARIES_SOULMATE_FILES_H_
