#pragma once
#include "weapons/Weapon.hpp"
#include <vector>
#include <raylib.h>

class Enemy;
class EnemyManager;
class Map;

class Sword : public Weapon {
public:
    Sword();
    void update(float dt, const Camera2D& gameCamera, bool playerFacingRight) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
    Vector2 getKnockback(bool facingRight) const override;
};

class Dagger : public Weapon {
public:
    Dagger();
    void update(float dt, const Camera2D& gameCamera, bool playerFacingRight) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
    Vector2 getKnockback(bool facingRight) const override;
    
private:
    float attackSpeedMultiplier = 2.0f;
};

class Spear : public Weapon {
public:
    Spear();
    void update(float dt, const Camera2D& gameCamera, bool playerFacingRight) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
    Vector2 getKnockback(bool facingRight) const override;
    
private:
    float extraRange = 1.5f;
};

class Bow : public Weapon {
public:
    Bow();
    ~Bow();
    void update(float dt, const Camera2D& gameCamera, bool playerFacingRight) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
    bool isCharging() const;
    bool isFullyCharged() const;
    void fireArrow(Vector2 position, Vector2 direction);
    void updateArrows(float dt, const Map& map);
    void updateArrowsWithSubsteps(float dt, EnemyManager& enemyManager, int substeps, const Map& map);
    void drawArrows() const;
    void checkArrowCollisions(EnemyManager& enemyManager);
    void updatePosition(Vector2 newPosition);
    bool hasActiveArrows() const;
    Vector2 getKnockback(bool facingRight) const override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;

    class Arrow {
    public:
        Vector2 position;
        Vector2 prevPosition;
        Vector2 direction;
        float speed = 0.0f;
        float lifetime = 1.0f;
        bool active = true;
        bool wasFullyCharged = false;
        float particleTimer = 0.0f;
        Arrow() = default;
        Arrow(Vector2 pos, Vector2 dir, float spd = 400.0f, float life = 1.0f, bool fullyCharged = false)
            : position(pos), prevPosition(pos), direction(dir), speed(spd), lifetime(life), active(true), wasFullyCharged(fullyCharged), particleTimer(0.0f) {}
        Rectangle getHitbox() const;
    };

private:
    float chargeTime = 0.0f;
    bool charging = false;
    float maxChargeTime = 1.0f;
    float minChargeTime = 0.2f;
    mutable Vector2 position = {0, 0};
    std::vector<Arrow> activeArrows;
    Texture2D arrowTexture;
    Camera2D defaultCamera = { 0 };
};