// Copyright (2018) Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATEOTA_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATEOTA_H_

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <time.h>
#include "./SoulmateMain.h"
#include "./SoulmateCircadian.h"

int updateStartHour = 4;

namespace SoulmateOTA {
  int lastMinute = -1;
  int previousHour = -1;
  bool updating = false;

  void updateFromURL() {
    if (updating) return;

    updating = true;
    String url = "http://soulmate-server.herokuapp.com/stream/" + (String)FIRMWARE_NAME + "/" + (String)SOULMATE_VERSION;
    WiFiClient client;
    client.setTimeout(120 * 1000);

    t_httpUpdate_return ret = httpUpdate.update(client, url);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        updating = false;
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        updating = false;
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        updating = false;
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  }

  void check() {
    if (updating) return;
    int currentHour = hourNow(Circadian::startedSeconds);

    if (previousHour == updateStartHour - 1 && currentHour == updateStartHour) {
      Serial.println("Go time.");
      updateFromURL();
    }

    previousHour = currentHour;
  }
}

#endif // BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_
