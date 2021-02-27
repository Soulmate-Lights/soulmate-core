// Copyright (2018) Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATEOTA_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATEOTA_H_

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <time.h>
#include "SoulmateCircadian.h"

namespace SoulmateOTA {
  // What time of the day do we start updates?
  static int updateStartHour = 4;

  int lastMinute = -1;
  int previousHour = -1;
  bool updating = false;
  bool clockSet = false;

  void update() {
    if (updating) return;

    Serial.println("Starting OTA update...");

    String server = "http://soulmate-server.herokuapp.com/stream/";
    WiFiClient client;

    client.setTimeout(360 * 1000);
    String url = server + FIRMWARE_NAME + "/" + SOULMATE_VERSION;
    t_httpUpdate_return ret = httpUpdate.update(client, url);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        updating = false;
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        ESP.restart();
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

    updating = false;
  }

  bool shouldUpdate() {
    if (updating) return false;

    bool result;

    int currentHour = hourNow(Circadian::startedSeconds);

    result = previousHour == updateStartHour - 1 && currentHour == updateStartHour;

    previousHour = currentHour;

    return result;
  }
}

#endif // BUILDER_LIBRARIES_SOULMATE_SOULMATEMAIN_H_
