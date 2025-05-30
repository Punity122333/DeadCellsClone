#ifndef FISHEYE_GRADIENT_HPP
#define FISHEYE_GRADIENT_HPP

#include <raylib.h>


Color MyColorLerp(Color c1, Color c2, float t);

Texture2D CreateFisheyeGradient(int width, int height, Color innerColor, Color outerColor);

#endif 