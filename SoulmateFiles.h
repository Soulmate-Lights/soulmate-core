// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_FILES_H_
#define BUILDER_LIBRARIES_SOULMATE_FILES_H_
#include "FS.h"

#ifdef ESP32
  #include <SPIFFS.h>
#endif

// TODO(elliott): Change these to use "preferences" instead of SPIFFS

void writeFile(String name, String contents) {
  #if defined(ESP32) || defined(ESP8266)
    File f = SPIFFS.open(name, "w");
    f.print(contents);
    f.flush();
    f.close();
  #endif
}

String readFile(String name) {
  String ret;
  #if defined(ESP32) || defined(ESP8266)
    File f = SPIFFS.open(name, "r");
    if (!f) return "";
    ret = f.readString();
    f.close();
  #endif
  return ret;
}

#endif  // BUILDER_LIBRARIES_SOULMATE_FILES_H_
