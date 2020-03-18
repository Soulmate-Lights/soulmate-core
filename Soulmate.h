// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATE_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATE_H_

#include "./SoulmateMain.h"

#ifdef ESP32
  #ifndef SKIP_BLUETOOTH
    #include "./SoulmateBLE.h"
  #endif
#endif

#ifdef CORE_TEENSY
  #include "./SoulmateTeensy.h"
#endif

#include "./SoulmateStats.h"
#include "./SoulmateBeatSin.h"
#include "./SoulmateFiles.h"

#ifdef ESP32
  #include "./SoulmateWifi.h"
#endif

#include "./SoulmateGrid.h"
#include "./SoulmateMatrix.h"

#endif  // BUILDER_LIBRARIES_SOULMATE_SOULMATE_H_
