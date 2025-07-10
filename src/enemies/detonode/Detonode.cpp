#include "enemies/Detonode.hpp"
#include <raylib.h>

#include <cmath>

Detonode::Detonode(Vector2 pos)
    : Enemy(pos, 60, 60, 120.0f),
      currentState(IDLE),
      stateTimer(0.0f),
      bobOffset(0.0f),
      bobSpeed(2.0f),
      bobAmplitude(8.0f),
      blinkTimer(0.0f),
      blinkInterval(1.0f),
      blinkCount(0),
      explosionRadius(128.0f),
      detectionRange(200.0f),
      approachDistance(64.0f),
      blinkDuration(3.0f),
      lineOfSightTimer(0.0f),
      lastPlayerPosForLOS{0, 0},
      cachedLineOfSightResult(false)
{}

Detonode::Detonode(Detonode&& other) noexcept
    : Enemy(std::move(other)),
      currentState(other.currentState),
      stateTimer(other.stateTimer),
      bobOffset(other.bobOffset),
      bobSpeed(other.bobSpeed),
      bobAmplitude(other.bobAmplitude),
      blinkTimer(other.blinkTimer),
      blinkInterval(other.blinkInterval),
      blinkCount(other.blinkCount),
      explosionRadius(other.explosionRadius),
      detectionRange(other.detectionRange),
      approachDistance(other.approachDistance),
      blinkDuration(other.blinkDuration),
      lineOfSightTimer(other.lineOfSightTimer),
      lastPlayerPosForLOS(other.lastPlayerPosForLOS),
      cachedLineOfSightResult(other.cachedLineOfSightResult)
{}

Detonode& Detonode::operator=(Detonode&& other) noexcept {
    if (this != &other) {
        Enemy::operator=(std::move(other));
        currentState = other.currentState;
        stateTimer = other.stateTimer;
        bobOffset = other.bobOffset;
        bobSpeed = other.bobSpeed;
        bobAmplitude = other.bobAmplitude;
        blinkTimer = other.blinkTimer;
        blinkInterval = other.blinkInterval;
        blinkCount = other.blinkCount;
        explosionRadius = other.explosionRadius;
        detectionRange = other.detectionRange;
        approachDistance = other.approachDistance;
        blinkDuration = other.blinkDuration;
        lineOfSightTimer = other.lineOfSightTimer;
        lastPlayerPosForLOS = other.lastPlayerPosForLOS;
        cachedLineOfSightResult = other.cachedLineOfSightResult;
    }
    return *this;
}

Rectangle Detonode::getHitbox() const {
    return Rectangle{
        position.x,
        position.y,
        32.0f,
        32.0f
    };
}

EnemySpawnConfig Detonode::getSpawnConfig() const {
    return {0.3f, 1, true};
}

