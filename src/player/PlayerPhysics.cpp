#include "Player.hpp"
#include <raylib.h>

void Player::handleCollisions(Vector2& nextPos, const Map& map, float dt) {
    Rectangle playerHitbox = { nextPos.x + hitboxOffsetX, nextPos.y + hitboxOffsetY, hitboxWidth, hitboxHeight };
    onGround = false;

    int tileXStart = (int)(playerHitbox.x / 32) - 1;
    int tileYStart = (int)(playerHitbox.y / 32) - 1;
    int tileXEnd = tileXStart + 3;
    int tileYEnd = tileYStart + 4;

    if (dropTimer <= 0.0f) {
        for (int y = tileYStart; y <= tileYEnd; y++) {
            for (int x = tileXStart; x <= tileXEnd; x++) {
                if (!map.isSolidTile(x, y)) continue;

                bool climbingUp = onLadder && (velocity.y < 0);
                bool headInsideTile = (playerHitbox.y <= y * 32.0f + 31) && (playerHitbox.y > y * 32.0f - hitboxHeight + 1);
                if (climbingUp && headInsideTile) continue;

                bool isBelowTile = (y * 32.0f >= playerHitbox.y + playerHitbox.height) &&
                                   (x == (int)((playerHitbox.x + playerHitbox.width / 2) / 32));
                if (dropTimer > 0.0f && isBelowTile) continue;

                Rectangle tileRect = { x * 32.0f, y * 32.0f, 32.0f, 32.0f };
                if (CheckCollisionRecs(playerHitbox, tileRect)) {
                    Rectangle intersection = GetCollisionRec(playerHitbox, tileRect);

                    bool isHorizontalCollision = intersection.width < intersection.height;
                    bool isMovingTowardsTile = (velocity.x > 0 && playerHitbox.x < tileRect.x) ||
                                               (velocity.x < 0 && playerHitbox.x > tileRect.x);
                    bool isNearGroundLevelOfTile = (playerHitbox.y + playerHitbox.height >= tileRect.y) &&
                                                   (playerHitbox.y + playerHitbox.height <= tileRect.y + 32 + 5); 

                    if (isHorizontalCollision && isMovingTowardsTile && isNearGroundLevelOfTile && onGround) {
                        if (!map.isSolidTile(x, y - 1)) {
                            int targetXTile = (int)((playerHitbox.x + playerHitbox.width / 2) / 32); 
                            if (!map.isSolidTile(targetXTile, y - 1)) { 
                                nextPos.y = tileRect.y - hitboxHeight - hitboxOffsetY;
                                if (velocity.x > 0) {
                                    nextPos.x = tileRect.x - hitboxWidth - hitboxOffsetX;
                                } else {
                                    nextPos.x = tileRect.x + 32 - hitboxOffsetX;
                                }
                                velocity.y = 0;
                                onGround = true;
                                playerHitbox.x = nextPos.x + hitboxOffsetX;
                                playerHitbox.y = nextPos.y + hitboxOffsetY;
                                continue;
                            }
                        }
                    }

                    if (isHorizontalCollision) {
                        if (playerHitbox.x < tileRect.x)
                            nextPos.x -= intersection.width;
                        else
                            nextPos.x += intersection.width;
                        velocity.x = 0;
                    } else { 
                        if (playerHitbox.y < tileRect.y) {
                            nextPos.y -= intersection.height;
                            onGround = true;
                        } else {
                            nextPos.y += intersection.height;
                        }
                        velocity.y = 0;
                    }

                    playerHitbox.x = nextPos.x + hitboxOffsetX;
                    playerHitbox.y = nextPos.y + hitboxOffsetY;
                }
            }
        }
    }
}