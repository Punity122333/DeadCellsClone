#pragma once
#include <raylib.h>
#include <string>

class ScrapHound;
class Automaton;

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
    virtual void update(float dt, const Camera2D& gameCamera, bool playerFacingRight);
    virtual void startAttack();
    virtual void draw(Vector2 playerPosition, bool facingRight) const;
    virtual Rectangle getHitbox(Vector2 playerPosition, bool facingRight) const;
    bool isAttacking() const { return attacking; }
    std::string getName() const { return name; }
    WeaponType getType() const { return type; }
    float getDamage() const { return baseDamage * damageMultiplier; }
    float getAttackSpeed() const { return attackSpeed; }
    float getRange() const { return range; }
    int getLevel() const { return level; }
    virtual void levelUp();
    virtual Vector2 getKnockback(bool facingRight) const;
protected:
    std::string name;
    WeaponType type;
    float baseDamage;
    float attackSpeed;
    float range;
    float attackTimer = 0.0f;
    bool attacking = false;
    int level = 1;
    int comboCount = 0;
    float damageMultiplier = 1.0f;
    Texture2D texture;
    int currentFrame = 0;
    float frameTime = 0.0f;
};