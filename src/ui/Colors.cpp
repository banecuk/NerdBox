#include "Colors.h"

//<<constructor>>
Colors::Colors()
{
  generateGradient();
}

//<<destructor>>
Colors::~Colors()
{
}

uint16_t Colors::getColorFromPercent(byte value, bool dim)
{
  if (value>99) {
    value = 99;
  }
  uint16_t result;
  if (dim) {
    result = COLOR_GRADIENT_DIM[value];
  } else {
    result = COLOR_GRADIENT[value];
  }
  return result;
}

uint16_t Colors::generateColorFromPercent(byte value)
{
  uint16_t blue = 0x09ea; 
  uint16_t green = 0x3ba2; //0x3363; //0x22c6;
  uint16_t yellow = 0x9CC0;
  uint16_t red = 0xF800;
  uint16_t C1, C2;
  uint8_t alpha;

  if (value < 25) {
    C1 = blue;
    C2 = green;
    alpha = value * 10; // Approximate linear scaling (25 * 4 = 100)
  } else if (value < 60) {
    C1 = green;
    C2 = yellow;
    alpha = (value - 25) * 7; // Approximate linear scaling (35 * 2.85714)
  } else {
    C1 = yellow;
    C2 = red;
    alpha = (value - 60) * 6.375; // 255/40 = 6.375
  }
  return blendRgb565(C1, C2, alpha);
}

void Colors::generateGradient()
{
  for (int i=0; i<100; i++) {
    COLOR_GRADIENT[i] = generateColorFromPercent(i);
    COLOR_GRADIENT_DIM[i] = blendRgb565(COLOR_GRADIENT[i], 0x00, 48);
  }
}

uint16_t Colors::getColorFromPercent30plus(byte value, bool dim) {
  if (value > 29) {
    value = trunc((value - 30) * 1.42857 + 0.5);
  } else {
    value = 0;
  }
  const uint16_t* gradient = dim ? COLOR_GRADIENT_DIM : COLOR_GRADIENT;
  return gradient[value];
}

#define MAKE_RGB565(r, g, b) ((r << 11) | (g << 5) | (b))

uint16_t Colors::blendRgb565(uint16_t a, uint16_t b, uint8_t Alpha)
{
  const uint8_t invAlpha = 255 - Alpha;

  uint16_t A_r = a >> 11;
  uint16_t A_g = (a >> 5) & 0x3f;
  uint16_t A_b = a & 0x1f;

  uint16_t B_r = b >> 11;
  uint16_t B_g = (b >> 5) & 0x3f;
  uint16_t B_b = b & 0x1f;

  uint32_t C_r = (A_r * invAlpha + B_r * Alpha) / 255;
  uint32_t C_g = (A_g * invAlpha + B_g * Alpha) / 255;
  uint32_t C_b = (A_b * invAlpha + B_b * Alpha) / 255;

  return MAKE_RGB565(C_r, C_g, C_b);
}

uint16_t Colors::darken(uint16_t color, uint8_t alpha)
{
  return blendRgb565(color, 0x00, alpha);
}
