// Copyright 2019 Soulmate Lighting, LLC

#ifndef SOULMATE_STATS_H_
#define SOULMATE_STATS_H_
namespace Stats {
  int rand_range(int min, int max) {
    return random(min, (max + 1));
  }
} // namespace Stats

namespace Util {
  int rand_range(int min, int max) {
    return random(min, (max + 1));
  }
} // namespace Util
#endif
