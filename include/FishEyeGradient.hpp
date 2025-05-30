#ifndef FISHEYE_GRADIENT_HPP
#define FISHEYE_GRADIENT_HPP

#include <raylib.h>

// Function declaration for linear color interpolation
Color MyColorLerp(Color c1, Color c2, float t);

// Function declaration for creating the fisheye gradient texture
Texture2D CreateFisheyeGradient(int width, int height, Color innerColor, Color outerColor);

#endif // FISHEYE_GRADIENT_HPP