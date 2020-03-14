// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_MATRIX_H_
#define BUILDER_LIBRARIES_SOULMATE_MATRIX_H_

// A matrix assuming 13x13 grid.
// From now on, all patterns should use this!

// Usage in Soulmate routines:
// void myRoutine() {
//   int width = Matrix::width;
//   int height = Matrix::height;
//   Matrix::leds[Matrix::XY(width, height)] = CRGB(255, 255, 255);
//   Matrix::print();
// }

#define kMatrixWidth LED_COLS
#define kMatrixHeight LED_ROWS
#define kBorderWidth 1

namespace Matrix {
  int NUM_LEDS = kMatrixHeight * kMatrixWidth;

  uint8_t CentreX =  (kMatrixWidth / 2) - 1;
  uint8_t CentreY = (kMatrixHeight / 2) - 1;

  // Function for returning XY coordinates
  uint16_t XY(uint8_t x, uint8_t y) {
    return (y * kMatrixWidth) + x;
  }

  // 16x16 (+1) matrix pixel array
  CRGB leds[kMatrixWidth * kMatrixHeight + 1];

  uint16_t mappedLeds[37] = {
    3, 5, 7, 9,
    28, 30, 32, 34, 36,
    53, 55, 57, 59, 61, 63,
    78, 80, 82, 84, 86, 88, 90,
    105, 107, 109, 111, 113, 115,
    132, 134, 136, 138, 140,
    159, 161, 163, 165
  };

  void setPixel(int x, int y, CRGB color) {
    int index = XY(x, y);
    leds[index] = color;
  }

  double gradientDistance(double startX, double startY, double endX, double endY, double angle) {
    double x = endX - startX;
    double y = endY - startY;
    double z = sqrt(sq(x) + sq(y));
    double a_degrees = atan(y / x) * 180.0 / M_PI;
    double c_degrees = (90.0 - angle - a_degrees);
    double result = z * sin(c_degrees * M_PI / 180.0);

    // Reverse because trigonometry is symmetrical
    if (x < 0) {
      result = -result;
    }

    return result;
  }

  // Neat per-pixel loop function that gives x and y and expects a colour.
  // Usage:
  // Matrix::forEach([&variableToCapture, &anotherVariableToCapture](int x, int y) -> CRGB {
  // Notes:
  // You need to "capture" variables inside a lambda functor thingie (variableToCapture in the example)
  // in order to use them in the anonymous function.
  void forEach(std::function<CRGB(int, int)> perPixel) {
    for (uint16_t y = 0; y < kMatrixHeight; y++) {
      for (uint16_t x = 0; x < kMatrixWidth; x++) {
        CRGB color = perPixel(x, y);
        setPixel(x, y, color);
      }
    }
  }

  void drawGradient(double angle, double xCenter, double yCenter, double hueDifference, uint8_t startingHue) {
    forEach([&angle, &xCenter, &yCenter, &hueDifference, &startingHue](int x, int y) -> CRGB {
      double distance = Matrix::gradientDistance(
        static_cast<double>(x),
        static_cast<double>(y),
        xCenter,
        yCenter,
        static_cast<double>(angle));

      double hue = distance * hueDifference;
      return CHSV(hue + startingHue, 255, 255);
    });
  }

  void print() {
    for (int z = 0; z < N_LEDS; z++) {
      // todo: return a variable instead of outputting directly here?
      Soulmate.led_arr[z] = leds[mappedLeds[z]];
    }
  }
}  // namespace Matrix

uint16_t XY(uint8_t x, uint8_t y) {
  return Matrix::XY(x, y);
}

float beatsin16Float(
  float beats_per_minute,
  float lowest = 0,
  float highest = 65535,
  uint32_t timebase = 0,
  uint16_t phase_offset = 0) {
    float multiplier = 1000;
    float result = (float)beatsin16(
      beats_per_minute * multiplier, lowest * multiplier, highest * multiplier, timebase, phase_offset);
    return result / multiplier;
}
  // accum88 beats_per_minute, uint16_t lowest = 0, uint16_t highest = 65535,
  //                              uint32_t timebase = 0, uint16_t phase_offset = 0

int16_t gridIndexHorizontal(int16_t x, int16_t y) {
  if (y > LED_ROWS) return -1;
  if (x > LED_COLS) return -1;
  if (x < 0) return -1;
  if (y < 0) return -1;

  int16_t index = 0;
  if (y % 2 == 1) {
    index = y * LED_COLS + x;
  } else {
    index = y * LED_COLS + LED_COLS - 1 - x;
  }
  if (index > -1 && index < N_LEDS) {
    return index;
  } else {
    return -1;
  }
}

// TODO(elliott): Rename this to something about left-right-left
int16_t gridIndex(int16_t x, int16_t y) {
  if (x == 0 && y == 0) {
    return 0;
  }

  int16_t index = 0;
  if (x % 2 == 0) {
    index = x * LED_ROWS + y;
  } else {
    index = x * LED_ROWS + LED_ROWS - 1 - y;
  }
  if (index > -1 && index < N_LEDS) {
    return index;
  } else {
    return -1;
  }
}

#endif  // BUILDER_LIBRARIES_SOULMATE_MATRIX_H_
