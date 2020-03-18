// Copyright 2018 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_GRID_H_
#define BUILDER_LIBRARIES_SOULMATE_GRID_H_

// Lights (physical):
//  X   0   1   2   3   X   X
//  X   4   5   6   7   8   X
//  9  10  11  12  13  14   X
// 15  16  17  18  19  20  21
// 22  23  24  25  26  27   X
//  X  28  29  30  31  32   X
//  X  33  34  35  36   X   X

// Cells (logical):
//   X    X   X   3   X   5   X   7   X   9   X   X   X
//   X    X   X   X   X   X   X   X   X   X   X   X   X
//   X    X   28  X   30  X   32  X   34  X   36  X   X
//   X    X   X   X   X   X   X   X   X   X   X   X   X
//   X    53  X   55  X   57  X   59  X   61  X   63  X
//   X    X   X   X   X   X   X   X   X   X   X   X   X
//   78   X   80  X   82  X   84  X   86  X   88  X   90
//   X    X   X   X   X   X   X   X   X   X   X   X   X
//   X    105 X   107 X   109 X   111 X   113 X   115 X
//   X    X   X   X   X   X   X   X   X   X   X   X   X
//   X    X   132 X   134 X   136 X   138 X   140 X   X
//   X    X   X   X   X   X   X   X   X   X   X   X   X
//   X    X   X   159 X   161 X   163 X   165 X   X   X

// Cells (logical):
//  0   1   2   3   4   5   6
//  7   8   9  10  11  12  13
// 14  15  16  17  18  19  20
// 21  22  23  24  25  26  27
// 28  29  30  31  32  33  34
// 35  36  37  38  39  40  41
// 42  43  44  45  46  47  48

namespace Grid {

  struct Point {
    int x, y;
  };

  struct Neighbors {
    struct Point v[8] = {Point{-1, -1}};
  };

  // Maps physical lights to their logical cells
  int n_hex_leds = 37;

  int hex[37] = {
    3, 5, 7, 9, 28, 30,
    32, 34, 36, 53, 55,
    57, 59, 61, 63, 78,
    80, 82, 84, 86, 88,
    90, 105, 107, 109, 111,
    113, 115, 132, 134, 136,
    138, 140, 159, 161, 163,
    165
  };

  int GetIndex(struct Point pt) {
    int v;
    if (pt.y % 2 != 0) {
      v = (LED_COLS - 1 - pt.x) + LED_COLS*pt.y;
    } else {
      v = pt.x + LED_COLS*pt.y;
    }

    return v;
  }

  struct Point GetPoint(int index) {
    Point out;

    if ((index > N_CELLS) || (index < 0)) {
      out.x = -1;
      out.y = -1;

      return out;
    }
    if (out.y % 2 != 0) {
      out.x = (LED_COLS - 1) - (index % LED_COLS);
      out.y = index / LED_COLS;

    } else {
      out.x = index % LED_COLS;
      out.y = index / LED_COLS;
    }

    return out;
  }

  struct Neighbors GetNeighbors(struct Point pt) {
    struct Neighbors out;

    int cnt = 0;
    for (int i =- 1; i < 2; i++) {
      for (int j =- 1; j < 2; j++) {
        if ((i == 0) && (j == 0)) {
          continue;
        }

        out.v[cnt] = GetPoint(GetIndex(Point{(pt.x+i), (pt.y+j)}));

        cnt++;
      }
    }

    return out;
  }

  int InfiniteGetIndex(struct Point pt) {
    if (pt.x >= LED_COLS) return -1;
    if (pt.x < 0) return -1;

    int v;
    if (pt.y % 2 != 0) {
      v = LED_COLS - 1 - pt.x + LED_COLS*pt.y;
    } else {
      v = pt.x + LED_COLS*pt.y;
    }

    return (v+N_CELLS)%N_CELLS;
  }

  struct Point InfiniteGetPoint(int index) {
    Point out;

    out.y = index / LED_COLS;

    index = index % N_CELLS;

    if (out.y % 2 != 0) {
      out.x = (LED_COLS - 1) - (index % LED_COLS);
      // out.y = index / LED_COLS;
    } else {
      out.x = index % LED_COLS;
      // out.y = index / LED_COLS;
    }

    return out;
  }

  struct Neighbors InfiniteGetNeighbors(struct Point pt) {
    struct Neighbors out;

    int cnt = 0;
    for (int i =- 1; i < 2; i++) {
      for (int j =- 1; j < 2; j++) {
        if ((i == 0) && (j == 0)) {
          continue;
        }

        int index = InfiniteGetIndex(Point{(pt.x+i), (pt.y+j)});

        // We have hit a wall
        if ((index) < 0) {
          out.v[cnt] = Point{-1, -1};
        } else {
          out.v[cnt] = InfiniteGetPoint(index);
        }

        cnt++;
      }
    }

    return out;
  }

  bool IsBottomRow(struct Point pt) {
    return pt.y == 0;
  }

  bool IsTopRow(struct Point pt) {
    return pt.y == (N_CELLS/LED_COLS-1);
  }
}  // namespace Grid

#endif  // BUILDER_LIBRARIES_SOULMATE_GRID_H_
