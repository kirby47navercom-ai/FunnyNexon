#pragma once

#include "Player.h"

#include <vector>

class Renderer;

struct Projectile {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float radius = 18.0f;
};

class Boss {
public:
    void Reset(int stage);
    void Update(float deltaTime, Player& player);
    void Draw(Renderer& renderer) const;
    void TakeGestureHit(int damage);

    bool IsDead() const { return m_hp <= 0; }
    int Hp() const { return m_hp; }
    int MaxHp() const { return m_maxHp; }

private:
    void SpawnProjectile(const Player& player);
    void DrawBody(Renderer& renderer) const;
    void DrawHp(Renderer& renderer) const;

private:
    int m_stage = 1;
    int m_hp = 240;
    int m_maxHp = 240;
    float m_x = 940.0f;
    float m_y = 360.0f;
    float m_frame = 0.0f;
    float m_attackTimer = 0.0f;
    float m_hitFlashTimer = 0.0f;
    std::vector<Projectile> m_projectiles;
};

