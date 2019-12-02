// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATE_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATE_H_

#include "SoulmateMain.h"

#ifdef ESP32
  #ifndef SKIP_BLUETOOTH
    #include "ble.h"
  #endif
#endif

#ifdef CORE_TEENSY
  #include "teensy.h"
#endif

#include "./stats.h"
#include "./beatsinfloat.h"
#include "./files.h"

#ifdef ESP32
  #include "SoulmateWifi.h"
#endif

#include "Grid.h"
#include "Matrix.h"

#endif  // BUILDER_LIBRARIES_SOULMATE_SOULMATE_H_
