// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_STATS_H_
#define BUILDER_LIBRARIES_SOULMATE_STATS_H_
namespace Stats {
  int rand_range(int min, int max) {
    return random(min, (max + 1));
  }
}

namespace Util {
  int rand_range(int min, int max) {
    return random(min, (max + 1));
  }
}
#endif