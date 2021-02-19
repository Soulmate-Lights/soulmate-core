#ifndef BUILDER_LIBRARIES_SOULMATE_TIME_H_
#define BUILDER_LIBRARIES_SOULMATE_TIME_H_

#include "Arduino.h"
#define TIME_API "http://worldtimeapi.org/api/ip"
#include "./components/ArduinoJson/ArduinoJson.h"
#include <HTTPClient.h>
#include <WiFi.h>

long fetchTime();
int dayOfWeek(double seconds, double secondsAtStart);
int hour(double seconds, double secondsAtStart);
int minute(double seconds, double secondsAtStart);
int dayOfWeekNow(double secondsAtStart);
int hourNow(double secondsAtStart);
int minuteNow(double secondsAtStart);

#endif
