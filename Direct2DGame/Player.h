#pragma once

#include "GameState.h"

class InputState;
class Renderer;

struct HitBox {
    float left = 0.0f;
    float bottom = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
};

class Player {
public:
    void Reset(const GameState& state, int stage);
    void Update(const InputState& input, float deltaTime);
    void Draw(Renderer& renderer) const;

    void StartAttack();
    bool TakeDamage(int amount);

    bool IsDead() const { return m_hp <= 0; }
    int Hp() const { return m_hp; }
    int MaxHp() const { return m_maxHp; }
    int AttackPower() const { return m_attackPower; }
    float X() const { return m_x; }
    float Y() const { return m_y; }
    HitBox Bounds() const;

private:
    float GroundForStage(int stage) const;

private:
    float m_x = 640.0f;
    float m_y = 100.0f;
    float m_velocityY = 0.0f;
    float m_ground = 100.0f;

    bool m_facingLeft = false;
    bool m_grounded = true;
    bool m_moving = false;

    int m_hp = 3;
    int m_maxHp = 3;
    int m_attackPower = 20;

    float m_frame = 0.0f;
    float m_attackTimer = 0.0f;
    float m_invincibleTimer = 0.0f;
};

