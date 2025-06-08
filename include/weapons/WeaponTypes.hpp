#pragma once
#include "weapons/Weapon.hpp"
#include <vector>

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
    void update(float dt, const Camera2D& gameCamera, bool playerFacingRight) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;
    bool isCharging() const ;
    void fireArrow(Vector2 position, Vector2 direction);
    void updateArrows(float dt);
    void updateArrowsWithSubsteps(float dt, std::vector<class ScrapHound>& enemies, int substeps);
    void drawArrows() const;
    void checkArrowCollisions(std::vector<class ScrapHound>& enemies);
    void updatePosition(Vector2 newPosition);
    bool hasActiveArrows() const;
    Vector2 getKnockback(bool facingRight) const override;
private:
    float chargeTime = 0.0f;
    bool charging = false;
    float maxChargeTime = 1.0f;
    mutable Vector2 position;
    struct Arrow {
    Vector2 position;
    Vector2 prevPosition;
    Vector2 direction;
    float speed;
    float lifetime;
    bool active;
    bool fullyCharged;  
};
    std::vector<Arrow> activeArrows;
    Texture2D arrowTexture;
    Camera2D defaultCamera = { 0 };
    float minChargeTime = 0.2f;
};