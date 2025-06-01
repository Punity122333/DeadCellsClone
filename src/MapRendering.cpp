#include "Map.hpp"
#include <cmath> // For fmod, std::max, std::min

void Map::draw() const {
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (tiles[x][y] == 1 || tiles[x][y] == 6) {
                bool top    = (y > 0)            && (tiles[x][y - 1] == 1 || tiles[x][y - 1] == 6);
                bool bottom = (y < height - 1)   && (tiles[x][y + 1] == 1 || tiles[x][y + 1] == 6);
                bool left   = (x > 0)            && (tiles[x - 1][y] == 1 || tiles[x - 1][y] == 6);
                bool right  = (x < width - 1)    && (tiles[x + 1][y] == 1 || tiles[x + 1][y] == 6);
                bool topLeft     = (x > 0 && y > 0)                   && (tiles[x - 1][y - 1] == 1 || tiles[x - 1][y - 1] == 6);
                bool topRight    = (x < width - 1 && y > 0)           && (tiles[x + 1][y - 1] == 1 || tiles[x + 1][y - 1] == 6);
                bool bottomLeft  = (x > 0 && y < height - 1)          && (tiles[x - 1][y + 1] == 1 || tiles[x - 1][y + 1] == 6);
                bool bottomRight = (x < width - 1 && y < height - 1)  && (tiles[x + 1][y + 1] == 1 || tiles[x + 1][y + 1] == 6);

                int idx = 0;

                if (!top && !left && !right && !bottom) idx = 15;
                else if (!top && !left && !right)       idx = 4;
                else if (!top && !left && !bottom)      idx = 5;
                else if (!top && !right && !bottom)     idx = 7;
                else if (!left && !right && !bottom)    idx = 24;
                else if (!top && !left)                 idx = 0;
                else if (!top && !right)                idx = 3;
                else if (!top)                          idx = 1;
                else if (!right && !left)               idx = 14;
                else if (!right && !bottom)             idx = 103;
                else if (!left && !bottom)              idx = 104;
                else if (!right)                        idx = 13;
                else if (!left)                         idx = 10;
                else if (!bottom)                       idx = 105;
                else                                    idx = 11;

                DrawTexture(tileTextures[idx], x * 32, y * 32, WHITE);
            }
            else if (tiles[x][y] == 4 || tiles[x][y] == 7) {
                float alpha = transitionTimers[x][y] / GLITCH_TIME;
                if (alpha > 1.0f) alpha = 1.0f;
                int r = GetRandomValue(180, 255);
                int g = GetRandomValue(0, 255);
                int b = GetRandomValue(180, 255);
                Color glitchColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(alpha * 255) };
                DrawRectangle(x * 32, y * 32, 32, 32, glitchColor);
            }
            else if (tiles[x][y] == 5) {
                float alpha = 1.0f - (transitionTimers[x][y] / GLITCH_TIME);
                if (alpha < 0.0f) alpha = 0.0f;
                int r = GetRandomValue(0, 255);
                int g = GetRandomValue(180, 255);
                int b = GetRandomValue(0, 180);
                Color glitchColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(alpha * 255) };
                DrawRectangle(x * 32, y * 32, 32, 32, glitchColor);
            }
            else if (tiles[x][y] == TILE_HIGHLIGHT_CREATE) {
                float blink_progress = fmod(transitionTimers[x][y], BLINK_CYCLE_TIME);
                float alpha_normalized;
                if (blink_progress < BLINK_CYCLE_TIME / 2.0f) {
                    alpha_normalized = blink_progress / (BLINK_CYCLE_TIME / 2.0f);
                } else {
                    alpha_normalized = 1.0f - ((blink_progress - BLINK_CYCLE_TIME / 2.0f) / (BLINK_CYCLE_TIME / 2.0f));
                }
                alpha_normalized = std::max(0.0f, std::min(1.0f, alpha_normalized));

                float alpha;
                if (alpha_normalized < 0.5f) {
                    alpha = 1.0f - (alpha_normalized / 0.5f) * (1.0f - MIN_HIGHLIGHT_OPACITY);
                } else {
                    alpha = MIN_HIGHLIGHT_OPACITY + ((alpha_normalized - 0.5f) / 0.5f) * (1.0f - MIN_HIGHLIGHT_OPACITY);
                }
                alpha = std::max(MIN_HIGHLIGHT_OPACITY, std::min(1.0f, alpha));

                Color highlightColor = { 0, 255, 0, (unsigned char)(alpha * 255) };
                
                bool top    = (y > 0)            && (tiles[x][y - 1] == 1 || tiles[x][y - 1] == 6);
                bool bottom = (y < height - 1)   && (tiles[x][y + 1] == 1 || tiles[x][y + 1] == 6);
                bool left   = (x > 0)            && (tiles[x - 1][y] == 1 || tiles[x - 1][y] == 6);
                bool right  = (x < width - 1)    && (tiles[x + 1][y] == 1 || tiles[x + 1][y] == 6);
                bool topLeft     = (x > 0 && y > 0)                   && (tiles[x - 1][y - 1] == 1 || tiles[x - 1][y - 1] == 6);
                bool topRight    = (x < width - 1 && y > 0)           && (tiles[x + 1][y - 1] == 1 || tiles[x + 1][y - 1] == 6);
                bool bottomLeft  = (x > 0 && y < height - 1)          && (tiles[x - 1][y + 1] == 1 || tiles[x - 1][y + 1] == 6);
                bool bottomRight = (x < width - 1 && y < height - 1)  && (tiles[x + 1][y + 1] == 1 || tiles[x + 1][y + 1] == 6);

                int idx = 0;

                if (!top && !left && !right && !bottom) idx = 15;
                else if (!top && !left && !right)       idx = 4;
                else if (!top && !left && !bottom)      idx = 5;
                else if (!top && !right && !bottom)     idx = 7;
                else if (!left && !right && !bottom)    idx = 24;
                else if (!top && !left)                 idx = 0;
                else if (!top && !right)                idx = 3;
                else if (!top)                          idx = 1;
                else if (!right && !left)               idx = 14;
                else if (!right && !bottom)             idx = 103;
                else if (!left && !bottom)              idx = 104;
                else if (!right)                        idx = 13;
                else if (!left)                         idx = 10;
                else if (!bottom)                       idx = 105;
                else                                    idx = 11;

                DrawTexture(tileTextures[idx], x * 32, y * 32, highlightColor);
            }
            else if (tiles[x][y] == TILE_HIGHLIGHT_DELETE) {
                float blink_progress = fmod(transitionTimers[x][y], BLINK_CYCLE_TIME);
                float alpha_normalized;
                if (blink_progress < BLINK_CYCLE_TIME / 2.0f) {
                    alpha_normalized = blink_progress / (BLINK_CYCLE_TIME / 2.0f);
                } else {
                    alpha_normalized = 1.0f - ((blink_progress - BLINK_CYCLE_TIME / 2.0f) / (BLINK_CYCLE_TIME / 2.0f));
                }
                alpha_normalized = std::max(0.0f, std::min(1.0f, alpha_normalized));

                float alpha;
                if (alpha_normalized < 0.5f) {
                    alpha = 1.0f - (alpha_normalized / 0.5f) * (1.0f - MIN_HIGHLIGHT_OPACITY);
                } else {
                    alpha = MIN_HIGHLIGHT_OPACITY + ((alpha_normalized - 0.5f) / 0.5f) * (1.0f - MIN_HIGHLIGHT_OPACITY);
                }
                alpha = std::max(MIN_HIGHLIGHT_OPACITY, std::min(1.0f, alpha));

                Color highlightColor = { 255, 0, 0, (unsigned char)(alpha * 255) };
                
                bool top    = (y > 0)            && (tiles[x][y - 1] == 1 || tiles[x][y - 1] == 6);
                bool bottom = (y < height - 1)   && (tiles[x][y + 1] == 1 || tiles[x][y + 1] == 6);
                bool left   = (x > 0)            && (tiles[x - 1][y] == 1 || tiles[x - 1][y] == 6);
                bool right  = (x < width - 1)    && (tiles[x + 1][y] == 1 || tiles[x + 1][y] == 6);
                bool topLeft     = (x > 0 && y > 0)                   && (tiles[x - 1][y - 1] == 1 || tiles[x - 1][y - 1] == 6);
                bool topRight    = (x < width - 1 && y > 0)           && (tiles[x + 1][y - 1] == 1 || tiles[x + 1][y - 1] == 6);
                bool bottomLeft  = (x > 0 && y < height - 1)          && (tiles[x - 1][y + 1] == 1 || tiles[x - 1][y + 1] == 6);
                bool bottomRight = (x < width - 1 && y < height - 1)  && (tiles[x + 1][y + 1] == 1 || tiles[x + 1][y + 1] == 6);

                int idx = 0;

                if (!top && !left && !right && !bottom) idx = 15;
                else if (!top && !left && !right)       idx = 4;
                else if (!top && !left && !bottom)      idx = 5;
                else if (!top && !right && !bottom)     idx = 7;
                else if (!left && !right && !bottom)    idx = 24;
                else if (!top && !left)                 idx = 0;
                else if (!top && !right)                idx = 3;
                else if (!top)                          idx = 1;
                else if (!right && !left)               idx = 14;
                else if (!right && !bottom)             idx = 103;
                else if (!left && !bottom)              idx = 104;
                else if (!right)                        idx = 13;
                else if (!left)                         idx = 10;
                else if (!bottom)                       idx = 105;
                else                                    idx = 11;

                DrawTexture(tileTextures[idx], x * 32, y * 32, highlightColor);
            } else if (tiles[x][y] == 2) {
                DrawRectangle(x * 32 + 10, y * 32, 12, 32, GOLD);
            }
            else if (tiles[x][y] == 3) {
                DrawRectangle(x * 32 + 14, y * 32, 4, 32, SKYBLUE);
            }
        }
    }
}
