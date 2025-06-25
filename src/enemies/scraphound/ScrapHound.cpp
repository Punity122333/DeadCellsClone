#include "enemies/ScrapHound.hpp"
#include "Pathfinding.hpp"
#include "map/Map.hpp"

#include <raymath.h>


#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>



static float Heuristic(int x1, int y1, int x2, int y2) {
    return fabsf((float)x1 - x2) + fabsf((float)y1 - y2);
}


ScrapHound::ScrapHound(Vector2 pos)
    : position(pos),
      velocity{0, 0},
      health(100.0f),
      maxHealth(100.0f),
      alive(true),
      invincibilityTimer(0.0f),
      hitEffectTimer(0.0f),
      currentColor(WHITE),
      isPounceCharging(false),
      isPouncing(false),
      pounceCharge(0.0f),
      pounceTimer(0.0f),
      pounceCooldown(0.0f),
      pounceTriggerDistance(250.0f),
      pounceChargeTime(1.5f),
      pounceForceX(600.0f),
      pounceForceY(-215.0f),
      pounceDuration(0.3f),
      pounceCooldownTime(2.0f),
      isMeleeCharging(false),
      isMeleeAttacking(false),
      meleeCharge(0.0f),
      meleeTimer(0.0f),
      meleeTriggerDistance(30.0f),
      meleeChargeTime(0.3f),
      meleeDuration(0.2f),
      pathfindingInProgress(false),
      pathReady(false),
      speed(150.0f),
      gravity(800.0f),
      pounceAnimRadius(0.0f),
      pounceAnimFade(0.0f)
{}

ScrapHound::ScrapHound(ScrapHound&& other) noexcept
    : position(other.position),
      velocity(other.velocity),
      health(other.health),
      maxHealth(other.maxHealth),
      alive(other.alive),
      isPounceCharging(other.isPounceCharging),
      isPouncing(other.isPouncing),
      pounceCharge(other.pounceCharge),
      pounceTimer(other.pounceTimer),
      pounceCooldown(other.pounceCooldown),
      pounceTriggerDistance(other.pounceTriggerDistance),
      pounceChargeTime(other.pounceChargeTime),
      pounceForceX(other.pounceForceX),
      pounceForceY(other.pounceForceY),
      pounceDuration(other.pounceDuration),
      pounceCooldownTime(other.pounceCooldownTime),
      isMeleeCharging(other.isMeleeCharging),
      isMeleeAttacking(other.isMeleeAttacking),
      meleeCharge(other.meleeCharge),
      meleeTimer(other.meleeTimer),
      meleeTriggerDistance(other.meleeTriggerDistance),
      meleeChargeTime(other.meleeChargeTime),
      meleeDuration(other.meleeDuration),
      pathfindingInProgress(other.pathfindingInProgress.load()),
      pathReady(other.pathReady.load()),
      pounceAnimRadius(other.pounceAnimRadius),
      pounceAnimFade(other.pounceAnimFade),
      invincibilityTimer(other.invincibilityTimer),
      hitEffectTimer(other.hitEffectTimer),
      currentColor(other.currentColor)
{
    std::lock_guard<std::mutex> lock(other.pathMutex);
    path = std::move(other.path);
}

ScrapHound& ScrapHound::operator=(ScrapHound&& other) noexcept {
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
        
        isPounceCharging = other.isPounceCharging;
        isPouncing = other.isPouncing;
        pounceCharge = other.pounceCharge;
        pounceTimer = other.pounceTimer;
        pounceCooldown = other.pounceCooldown;
        pounceTriggerDistance = other.pounceTriggerDistance;
        pounceChargeTime = other.pounceChargeTime;
        pounceForceX = other.pounceForceX;
        pounceForceY = other.pounceForceY;
        pounceDuration = other.pounceDuration;
        pounceCooldownTime = other.pounceCooldownTime;
        isMeleeCharging = other.isMeleeCharging;
        isMeleeAttacking = other.isMeleeAttacking;
        meleeCharge = other.meleeCharge;
        meleeTimer = other.meleeTimer;
        meleeTriggerDistance = other.meleeTriggerDistance;
        meleeChargeTime = other.meleeChargeTime;
        meleeDuration = other.meleeDuration;
        path = std::move(other.path);
        pathfindingInProgress = other.pathfindingInProgress.load();
        pathReady = other.pathReady.load();
        pounceAnimRadius = other.pounceAnimRadius;
        pounceAnimFade = other.pounceAnimFade;
    }
    return *this;
}

void ScrapHound::requestPathAsync(const Map& map, Vector2 start, Vector2 goal) {
    if (pathfindingInProgress) return;
    pathfindingInProgress = true;
    pathReady = false;

    std::thread([this, &map, start, goal]() {
        auto newPath = FindPathAStar(map, start, goal);
        {
            std::lock_guard<std::mutex> lock(pathMutex);
            path = std::move(newPath);
        }
        pathReady = true;
        pathfindingInProgress = false;
    }).detach();
}

Vector2 ScrapHound::getPosition() const {
    return position;
}

Rectangle ScrapHound::getArrowHitbox() const {
    
    return Rectangle{
        position.x - 28.0f, 
        position.y - 28.0f,
        56.0f, 
        56.0f  
    };
}

bool ScrapHound::canTakeDamage() const {
    return invincibilityTimer <= 0.0f;
}
