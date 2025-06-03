#pragma once
#include "weapons/Weapon.hpp"
#include <vector>

class Sword : public Weapon {
public:
    Sword();
    void update(float dt) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
};

class Dagger : public Weapon {
public:
    Dagger();
    void update(float dt) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
    
private:
    float attackSpeedMultiplier = 2.0f; // Daggers attack faster
};

class Spear : public Weapon {
public:
    Spear();
    void update(float dt) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
    
private:
    float extraRange = 1.5f; // Spears have more range
};

class Bow : public Weapon {
public:
    Bow();
    void update(float dt) override;
    void draw(Vector2 playerPosition, bool facingRight) const override;
    void startAttack() override;
    Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const override;
    
    // Arrow management
    void fireArrow(Vector2 position, Vector2 direction);
    void updateArrows(float dt);
    void drawArrows() const;
    void checkArrowCollisions(std::vector<class ScrapHound>& enemies);
    
private:
    float chargeTime = 0.0f;
    bool charging = false;
    float maxChargeTime = 1.0f;
    mutable Vector2 position; // Add this to store player position for arrow firing
    
    struct Arrow {
        Vector2 position;
        Vector2 direction;
        float speed;
        float lifetime;
        bool active;
    };
    
    std::vector<Arrow> activeArrows;
    Texture2D arrowTexture;
    Camera2D defaultCamera = { 0 }; // Default camera for when none is provided
};