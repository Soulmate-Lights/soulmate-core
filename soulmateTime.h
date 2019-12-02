#ifndef BUILDER_LIBRARIES_SOULMATE_TIME_H_
#define BUILDER_LIBRARIES_SOULMATE_TIME_H_

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

// String nameOfDay(int day) {
//   switch (day) {
//     case 0:
//       return "Sunday";
//       break;
//     case 1:
//       return "Monday";
//       break;
//     case 2:
//       return "Monday";
//       break;
//     case 3:
//       return "Tuesday";
//       break;
//     case 4:
//       return "Thursday";
//       break;
//     case 5:
//       return "Friday";
//       break;
//     case 6:
//       return "Saturday";
//       break;
//   }
//   return "None";
// }

#endif
