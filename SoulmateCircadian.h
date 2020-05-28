#include "./SoulmateFiles.h"
#include "./SoulmateTime.h"

#ifndef BUILDER_LIBRARIES_SOULMATE_CIRCADIAN_H_
  #define BUILDER_LIBRARIES_SOULMATE_CIRCADIAN_H_

namespace Circadian {
  const int SHOULD_TURN_OFF = 0;
  const int SHOULD_TURN_ON = 1;
  const int SHOULD_STAY_THE_SAME = 2;

  bool circadian = false;
  double wakeTime = 8;
  double sleepTime = 23;
  int lastMinute = -1;
  signed long startedSeconds;
  int previousHour = -1;
  bool trackingTime = false;

  void startTrackingTime(unsigned long startedSecondsValue) {
    startedSeconds = startedSecondsValue;
    trackingTime = true;
  }

  double readSavedWakeTime() {
    String savedWakeTime = readFile("/wakeTime.txt");
    if (!savedWakeTime.equals("")) {
      wakeTime = savedWakeTime.toDouble();
    }
    return wakeTime;
  }

  double readSavedSleepTime() {
    String savedSleepTime = readFile("/sleepTime.txt");
    if (!savedSleepTime.equals("")) {
      sleepTime = savedSleepTime.toDouble();
    }
    return sleepTime;
  }

  void setSleepTime(double newValue) {
    sleepTime = newValue;
    writeFile("/sleepTime.txt", String(newValue));
  }

  void setWakeTime(double newValue) {
    wakeTime = newValue;
    writeFile("/wakeTime.txt", String(newValue));
  }

  bool readSavedCircadian() {
    circadian = readFile("/circadian.txt") == "true";
    return circadian;
  }

  void saveCircadian(bool newCircadian) {
    circadian = newCircadian;
    writeFile("/circadian.txt", circadian ? "true" : "false");
  }

  int checkTime() {
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

  void setup() {
    readSavedCircadian();
    readSavedWakeTime();
    readSavedSleepTime();
  }
} // namespace Circadian

#endif
