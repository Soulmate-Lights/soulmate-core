// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_MATRIX_H_
#define BUILDER_LIBRARIES_SOULMATE_MATRIX_H_

// A matrix assuming 13x13 grid. This was used for the Hex prototype.
// Hex code might not compile without this but it eats memory

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

// namespace Matrix {
//   int NUM_LEDS = kMatrixHeight * kMatrixWidth;

//   int16_t CentreX = (kMatrixWidth / 2) - 1;
//   int16_t CentreY = (kMatrixHeight / 2) - 1;

  // Function for returning XY coordinates
  // uint16_t XY(uint8_t x, uint8_t y) {
  //   return (y * kMatrixWidth) + x;
  // }

  // 16x16 (+1) matrix pixel array
  // CRGB leds[kMatrixWidth * kMatrixHeight + 1];

  // clang-format off
  // uint16_t mappedLeds[37] = {
  //   3, 5, 7, 9,
  //   28, 30, 32, 34, 36,
  //   53, 55, 57, 59, 61, 63,
  //   78, 80, 82, 84, 86, 88, 90,
  //   105, 107, 109, 111, 113, 115,
  //   132, 134, 136, 138, 140,
  //   159, 161, 163, 165
  // };
  // clang-format on

  // void setPixel(int x, int y, CRGB color) {
  //   int index = XY(x, y);
  //   leds[index] = color;
  // }

  // double gradientDistance(double startX, double startY, double endX,
  //                         double endY, double angle) {
  //   double x = endX - startX;
  //   double y = endY - startY;
  //   double z = sqrt(sq(x) + sq(y));
  //   double a_degrees = atan(y / x) * 180.0 / M_PI;
  //   double c_degrees = (90.0 - angle - a_degrees);
  //   double result = z * sin(c_degrees * M_PI / 180.0);

  //   // Reverse because trigonometry is symmetrical
  //   if (x < 0) {
  //     result = -result;
  //   }

  //   return result;
  // }

  // Neat per-pixel loop function that gives x and y and expects a colour.
  // Usage:
  // Matrix::forEach([&variableToCapture, &anotherVariableToCapture](int x, int
  // y)
  // -> CRGB { Notes: You need to "capture" variables inside a lambda functor
  // thingie (variableToCapture in the example) in order to use them in the
  // anonymous function.
  // void forEach(std::function<CRGB(int, int)> perPixel) {
  //   for (uint16_t y = 0; y < kMatrixHeight; y++) {
  //     for (uint16_t x = 0; x < kMatrixWidth; x++) {
  //       CRGB color = perPixel(x, y);
  //       setPixel(x, y, color);
  //     }
  //   }
  // }

  // void drawGradient(double angle, double xCenter, double yCenter,
  //                   double hueDifference, uint8_t startingHue) {
  //   forEach([&angle, &xCenter, &yCenter, &hueDifference,
  //            &startingHue](int x, int y) -> CRGB {
  //     double distance = Matrix::gradientDistance(
  //         static_cast<double>(x), static_cast<double>(y), xCenter, yCenter,
  //         static_cast<double>(angle));

  //     double hue = distance * hueDifference;
  //     return CHSV(hue + startingHue, 255, 255);
  //   });
  // }

  // void print() {
  //   for (int z = 0; z < 37; z++) {
  //     // todo: return a variable instead of outputting directly here?
  //     Soulmate.leds[z] = leds[mappedLeds[z]];
  //   }
  // }
// } // namespace Matrix

// There are two gridIndex functions that work differently
// depending on the layout of your matrix. It's confusing.
// If your strips are vertical, use gridIndex, otherwise
// use gridIndexHorizontal. If it looks weird, switch 'em.

int16_t gridIndex(int16_t x, int16_t y) {
  if (y > LED_ROWS) return -1;
  if (x > LED_COLS) return -1;
  if (x < 0) return -1;
  if (y < 0) return -1;
  if (x == 0 && y == 0) return 0;

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

int16_t gridIndexHorizontal(uint8_t x, uint8_t y) {
  if (y > LED_ROWS) return -1;
  if (x > LED_COLS) return -1;
  if (x < 0) return -1;
  if (y < 0) return -1;

  int16_t xIndex = x;

  // Serpentine row
  bool oddRow = y % 2 != 1;
  if (SOULMATE_SERPENTINE && oddRow) {
    xIndex = LED_COLS - 1 - xIndex;
  }

  int16_t index = y * LED_COLS + xIndex;

  if (index > -1 && index < N_LEDS) {
    return index;
  }

  return -1;
}

uint16_t XY(uint8_t x, uint8_t y) {
  return gridIndexHorizontal(x, y);
}

#endif // BUILDER_LIBRARIES_SOULMATE_MATRIX_H_
