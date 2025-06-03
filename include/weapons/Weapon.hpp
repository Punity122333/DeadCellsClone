#pragma once
#include <raylib.h>
#include <string>

class ScrapHound;

enum class WeaponType {
    SWORD,
    DAGGER, 
    SPEAR,
    BOW,
    HAMMER,
    SHIELD
};

class Weapon {
public:
    Weapon(const std::string& name, WeaponType type, float baseDamage, float attackSpeed, float range);
    virtual ~Weapon() = default;
    
    // Core methods all weapons must implement
    virtual void update(float dt);
    virtual void draw(Vector2 playerPosition, bool facingRight) const;
    virtual void startAttack();
    virtual bool isAttacking() const;
    virtual Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const;
    
    // Getters
    std::string getName() const { return name; }
    WeaponType getType() const { return type; }
    float getDamage() const { return baseDamage * damageMultiplier; }
    float getAttackSpeed() const { return attackSpeed; }
    float getRange() const { return range; }
    int getLevel() const { return level; }
    
    // For weapon upgrades
    virtual void levelUp();
    
protected:
    std::string name;
    WeaponType type;
    float baseDamage;
    float attackSpeed; // Attacks per second
    float range;
    float attackTimer = 0.0f;
    bool attacking = false;
    int level = 1;
    int comboCount = 0;
    float damageMultiplier = 1.0f;
    
    // For animation
    Texture2D texture;
    int currentFrame = 0;
    float frameTime = 0.0f;
};