// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_FILES_H_
#define BUILDER_LIBRARIES_SOULMATE_FILES_H_
#include "FS.h"

#include <SPIFFS.h>

#include <Preferences.h>

void writeFile(String name, String contents);
String readFile(String name);
void deleteAllFiles();

#endif // BUILDER_LIBRARIES_SOULMATE_FILES_H_
