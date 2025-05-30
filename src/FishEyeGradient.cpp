#include "../include/FishEyeGradient.hpp"
#include <math.h> // For sqrtf
#include <raymath.h>

// Function definition for linear color interpolation
Color MyColorLerp(Color c1, Color c2, float t) {
    return (Color){
        (unsigned char)(c1.r + (c2.r - c1.r) * t),
        (unsigned char)(c1.g + (c2.g - c1.g) * t),
        (unsigned char)(c1.b + (c2.b - c1.b) * t),
        (unsigned char)(c1.a + (c2.a - c1.a) * t)
    };
}

// Function definition for creating the fisheye gradient texture
Texture2D CreateFisheyeGradient(int width, int height, Color innerColor, Color outerColor) {
    Image image = GenImageColor(width, height, BLANK);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float centerX = width / 2.0f;
            float centerY = height / 2.0f;
            float distance = sqrtf((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
            float maxDistance = sqrtf(centerX * centerX + centerY * centerY);

            float t = distance / maxDistance;
            t = Clamp(t, 0.0f, 1.0f);

            Color pixelColor = ColorLerp(innerColor, outerColor, t);
            ImageDrawPixel(&image, x, y, pixelColor);
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}