#ifndef BUILDER_LIBRARIES_SOULMATE_TIME_H_
#define BUILDER_LIBRARIES_SOULMATE_TIME_H_

String TIME_API = "http://worldtimeapi.org/api/ip";

#include "./components/ArduinoJson/ArduinoJson.h"
#include <HTTPClient.h>
#include <WiFi.h>

HTTPClient http;

long fetchTime() {
  long seconds = -1;

  http.begin(TIME_API);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, payload);
    JsonObject root = doc.as<JsonObject>();
    // JsonObject root = jsonBuffer.parseObject(payload);

    if (root.containsKey("datetime")) {
      const char *datetime = root.get<char *>("datetime");
      int Year, Month, Day, Hour, Minute, Second;
      sscanf(datetime, "%d-%d-%dT%d:%d:%d", &Year, &Month, &Day, &Hour, &Minute,
             &Second);

      seconds = Hour * 3600 + Minute * 60 + Second;
    }
  }

  http.end();
  return seconds;
}

int dayOfWeek(double seconds, double secondsAtStart) {
  double realTime = fmod(seconds + secondsAtStart, 604800);
  double day = realTime / 86400;
  return fmod(floor(day), 7);
}

int hour(double seconds, double secondsAtStart) {
  double realTime = fmod(seconds + secondsAtStart, 604800);
  int day = dayOfWeek(seconds, secondsAtStart);
  double dayOffset = day * 86400;
  double timeToday = realTime - dayOffset;
  double hourToday = timeToday / 60 / 60;
  return fmod(floor(hourToday), 24);
}

int minute(double seconds, double secondsAtStart) {
  double realTime = fmod(seconds + secondsAtStart, 604800);
  int day = dayOfWeek(seconds, secondsAtStart);
  double dayOffset = day * 86400;
  int hourValue = hour(seconds, secondsAtStart);
  double hourOffset = hourValue * 3600;
  double minuteValue = (realTime - hourOffset - dayOffset) / 60;
  return fmod(floor(minuteValue), 60);
}

int dayOfWeekNow(double secondsAtStart) {
  double seconds = millis() / 1000;
  return dayOfWeek(seconds, secondsAtStart);
}

int hourNow(double secondsAtStart) {
  double seconds = millis() / 1000;
  return hour(seconds, secondsAtStart);
}

int minuteNow(double secondsAtStart) {
  double seconds = millis() / 1000;
  return minute(seconds, secondsAtStart);
}

#endif
