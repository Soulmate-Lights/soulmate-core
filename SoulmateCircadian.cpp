// #ifndef BUILDER_LIBRARIES_SOULMATE_CIRCADIAN_CPP_
// #define BUILDER_LIBRARIES_SOULMATE_CIRCADIAN_CPP_

#include "./SoulmateCircadian.h"
#include "./SoulmateTime.h"
#include "./SoulmateFiles.h"

using namespace Circadian;

bool Circadian::circadian = false;
double Circadian::wakeTime = 8;
double Circadian::sleepTime = 23;
int Circadian::lastMinute = -1;
//  signed long startedSeconds;
signed long Circadian::startedSeconds;
int Circadian::previousHour = -1;
bool Circadian::trackingTime = false;


// namespace Circadian {
void Circadian::startTrackingTime(unsigned long startedSecondsValue) {
  startedSeconds = startedSecondsValue;
  trackingTime = true;
}

double Circadian::readSavedWakeTime() {
  String savedWakeTime = readFile("/wakeTime.txt");
  if (!savedWakeTime.equals("")) {
    wakeTime = savedWakeTime.toDouble();
  }
  return wakeTime;
}

double Circadian::readSavedSleepTime() {
  String savedSleepTime = readFile("/sleepTime.txt");
  if (!savedSleepTime.equals("")) {
    sleepTime = savedSleepTime.toDouble();
  }
  return sleepTime;
}

void Circadian::setSleepTime(double newValue) {
  sleepTime = newValue;
  writeFile("/sleepTime.txt", String(newValue));
}

void Circadian::setWakeTime(double newValue) {
  wakeTime = newValue;
  writeFile("/wakeTime.txt", String(newValue));
}

bool Circadian::readSavedCircadian() {
  circadian = readFile("/circadian.txt") == "true";
  return circadian;
}

void Circadian::saveCircadian(bool newCircadian) {
  circadian = newCircadian;
  writeFile("/circadian.txt", circadian ? "true" : "false");
}

int Circadian::checkTime() {
  int wakeHour = floor(wakeTime);
  int wakeMinute = ceil((wakeTime - (float)wakeHour) * 100.0);
  int sleepHour = floor(sleepTime);
  int sleepMinute = ceil((sleepTime - (float)sleepHour) * 100.0);

  // If we don't know what time it is, or circadian is turned off
  if (!trackingTime || !circadian) {
    return SHOULD_STAY_THE_SAME;
  }

  int currentHour = hourNow(startedSeconds);
  int currentMinute = minuteNow(startedSeconds);

  // Only check once per minute!
  if (currentMinute == lastMinute) {
    return SHOULD_STAY_THE_SAME;
  }

  lastMinute = currentMinute;

  previousHour = currentHour;

  // Turn on or off if it's the right hour and minute
  if (currentHour == wakeHour && currentMinute == wakeMinute) {
    return SHOULD_TURN_ON;
  } else if (currentHour == sleepHour && currentMinute == sleepMinute) {
    return SHOULD_TURN_OFF;
  }

  return SHOULD_STAY_THE_SAME;
}

void Circadian::setup() {
  readSavedCircadian();
  readSavedWakeTime();
  readSavedSleepTime();
}
// } // namespace Circadian

// #endif
