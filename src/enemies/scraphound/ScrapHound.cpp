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
    : Enemy(pos, 100, 100, 100.0f),
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
      pounceAnimRadius(0.0f),
      pounceAnimFade(0.0f),
      isPatrolling(false),
      patrolLeftBound(0.0f),
      patrolRightBound(0.0f),
      patrolDirection(1),
      playerDetectionRange(350.0f),
      patrolBoundsInitialized(false)
{}

ScrapHound::ScrapHound(ScrapHound&& other) noexcept
    : Enemy(std::move(other)),
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
      pounceAnimRadius(other.pounceAnimRadius),
      pounceAnimFade(other.pounceAnimFade),
      isPatrolling(other.isPatrolling),
      patrolLeftBound(other.patrolLeftBound),
      patrolRightBound(other.patrolRightBound),
      patrolDirection(other.patrolDirection),
      playerDetectionRange(other.playerDetectionRange),
      patrolBoundsInitialized(other.patrolBoundsInitialized)
{
    std::lock_guard<std::mutex> lock(other.pathMutex);
    path = std::move(other.path);
}

ScrapHound& ScrapHound::operator=(ScrapHound&& other) noexcept {
    if (this != &other) {
        Enemy::operator=(std::move(other));
        
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
        isPatrolling = other.isPatrolling;
        patrolLeftBound = other.patrolLeftBound;
        patrolRightBound = other.patrolRightBound;
        patrolDirection = other.patrolDirection;
        playerDetectionRange = other.playerDetectionRange;
        patrolBoundsInitialized = other.patrolBoundsInitialized;
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

Rectangle ScrapHound::getHitbox() const {
    return Rectangle{
        position.x - 16.0f,
        position.y - 16.0f,
        32.0f,
        32.0f
    };
}

EnemySpawnConfig ScrapHound::getSpawnConfig() const {
    return {0.6f, 2, false};
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

void ScrapHound::initializePatrolBounds(const Map& map) {
    if (patrolBoundsInitialized) return;
    
    float groundY = position.y + 32.0f; 
    patrolLeftBound = findPlatformLeftEdge(map, position.x, groundY);
    patrolRightBound = findPlatformRightEdge(map, position.x, groundY);

    patrolLeftBound += 16.0f;
    patrolRightBound -= 16.0f;

    if (patrolRightBound - patrolLeftBound < 32.0f) {
        float center = (patrolLeftBound + patrolRightBound) * 0.5f;
        patrolLeftBound = center - 16.0f;
        patrolRightBound = center + 16.0f;
    }
    
    patrolBoundsInitialized = true;
}

float ScrapHound::findPlatformLeftEdge(const Map& map, float startX, float y) const {
    float checkX = startX;
    const float TILE_SIZE = 32.0f;
    const float MIN_BOUND = 0.0f;

    while (checkX > MIN_BOUND) {

        bool hasGround = map.collidesWithGround({checkX, y});

        bool hasGroundAhead = map.collidesWithGround({checkX - TILE_SIZE, y});
        
        if (hasGround && !hasGroundAhead) {

            return checkX;
        }
        
        if (!hasGround) {

            return checkX + TILE_SIZE;
        }
        
        checkX -= TILE_SIZE;
    }
    
    return MIN_BOUND;
}

float ScrapHound::findPlatformRightEdge(const Map& map, float startX, float y) const {
    float checkX = startX;
    const float TILE_SIZE = 32.0f;
    const float MAX_BOUND = map.getWidth() * TILE_SIZE;
    

    while (checkX < MAX_BOUND) {

        bool hasGround = map.collidesWithGround({checkX, y});

        bool hasGroundAhead = map.collidesWithGround({checkX + TILE_SIZE, y});
        
        if (hasGround && !hasGroundAhead) {

            return checkX;
        }
        
        if (!hasGround) {

            return checkX - TILE_SIZE;
        }
        
        checkX += TILE_SIZE;
    }
    
    return MAX_BOUND;
}

void ScrapHound::updatePatrol(const Map& map, float dt) {
    if (!patrolBoundsInitialized) {
        initializePatrolBounds(map);
    }

    velocity.x = patrolDirection * speed * 0.5f; 

    bool shouldTurn = false;
    
    if (patrolDirection > 0) {

        if (position.x >= patrolRightBound) {
            shouldTurn = true;
        }

        else if (map.collidesWithGround({position.x + 32.0f + 16.0f, position.y + 16.0f})) {
            shouldTurn = true;
        }
    } else {

        if (position.x <= patrolLeftBound) {
            shouldTurn = true;
        }

        else if (map.collidesWithGround({position.x - 16.0f, position.y + 16.0f})) {
            shouldTurn = true;
        }
    }
    
    if (shouldTurn) {
        patrolDirection *= -1;
        velocity.x = patrolDirection * speed * 0.5f;
    }
}
