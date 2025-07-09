#include "enemies/Detonode.hpp"
#include <raylib.h>
#include <mutex>
#include <atomic>
#include <cmath>

Detonode::Detonode(Vector2 pos)
    : position(pos),
      velocity{0, 0},
      health(60),
      maxHealth(60),
      alive(true),
      invincibilityTimer(0.0f),
      hitEffectTimer(0.0f),
      currentColor(WHITE),
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
      speed(120.0f),
      blinkDuration(3.0f),
      pathfindingInProgress(false),
      pathReady(false),
      lineOfSightTimer(0.0f),
      lastPlayerPosForLOS{0, 0},
      cachedLineOfSightResult(false)
{}

Detonode::Detonode(Detonode&& other) noexcept
    : position(other.position),
      velocity(other.velocity),
      health(other.health),
      maxHealth(other.maxHealth),
      alive(other.alive),
      invincibilityTimer(other.invincibilityTimer),
      hitEffectTimer(other.hitEffectTimer),
      currentColor(other.currentColor),
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
      speed(other.speed),
      blinkDuration(other.blinkDuration),
      pathfindingInProgress(other.pathfindingInProgress.load()),
      pathReady(other.pathReady.load()),
      lineOfSightTimer(other.lineOfSightTimer),
      lastPlayerPosForLOS(other.lastPlayerPosForLOS),
      cachedLineOfSightResult(other.cachedLineOfSightResult)
{
    std::lock_guard<std::mutex> lock(other.pathMutex);
    path = std::move(other.path);
}

Detonode& Detonode::operator=(Detonode&& other) noexcept {
    if (this != &other) {
        std::lock_guard<std::mutex> lock1(pathMutex);
        std::lock_guard<std::mutex> lock2(other.pathMutex);

        position = other.position;
        velocity = other.velocity;
        health = other.health;
        maxHealth = other.maxHealth;
        alive = other.alive;
        invincibilityTimer = other.invincibilityTimer;
        hitEffectTimer = other.hitEffectTimer;
        currentColor = other.currentColor;
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
        speed = other.speed;
        blinkDuration = other.blinkDuration;
        path = std::move(other.path);
        pathfindingInProgress = other.pathfindingInProgress.load();
        pathReady = other.pathReady.load();
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

Vector2 Detonode::getPosition() const {
    return position;
}

