
#ifndef BUILDER_LIBRARIES_SOULMATE_CIRCADIAN_H_
#define BUILDER_LIBRARIES_SOULMATE_CIRCADIAN_H_

namespace Circadian {
  const int SHOULD_TURN_OFF = 0;
  const int SHOULD_TURN_ON = 1;
  const int SHOULD_STAY_THE_SAME = 2;

  extern bool circadian;
  extern double wakeTime;
  extern double sleepTime;
  extern int lastMinute;
  extern signed long startedSeconds;
  extern int previousHour;
  extern bool trackingTime;

  // bool circadian;
  // double wakeTime;
  // double sleepTime;
  // int lastMinute;
  // signed long startedSeconds;
  // int previousHour;
  // bool trackingTime;
  void startTrackingTime(unsigned long startedSecondsValue);
  double readSavedWakeTime();
  double readSavedSleepTime();
  void setSleepTime(double newValue);
  void setWakeTime(double newValue);
  bool readSavedCircadian();
  void saveCircadian(bool newCircadian);
  int checkTime();
  void setup();
} // namespace Circadian

#endif
