#pragma once

#include "GameState.h"
#include "Geometry.h"

class InputState;
class Renderer;

using HitBox = RectF;

class Player {
public:
    void Reset(const GameState& state, int stage);
    void Update(const InputState& input, float deltaTime, bool gestureOpen);
    void Draw(Renderer& renderer) const;

    void QueueSmash();
    bool TakeDamage(int amount);
    void SetGround(float ground) { m_ground = ground; }

    bool IsDead() const { return m_hp <= 0; }
    bool IsInvincible() const { return m_invincibleTimer > 0.0f; }
    bool IsRollInvincible() const;
    int Hp() const { return m_hp; }
    int MaxHp() const { return m_maxHp; }
    int AttackPower() const { return m_attackPower; }
    float X() const { return m_x; }
    float Y() const { return m_y; }
    HitBox Bounds() const;

private:
    enum Mode {
        Idle,
        Walk,
        Run,
        Jump,
        DoubleJump,
        Evade,
        Attack,
        Hit,
        Dead
    };

    float GroundForStage(int stage) const;
    void ChangeMode(Mode mode);
    void ApplyHorizontalMove(float direction, float speed, float deltaTime);

private:
    Mode m_mode = Idle;
    float m_x = 640.0f;
    float m_y = 100.0f;
    float m_velocityY = 0.0f;
    float m_ground = 100.0f;
    float m_lastATap = -10.0f;
    float m_lastDTap = -10.0f;
    float m_aliveTime = 0.0f;

    bool m_facingLeft = false;
    bool m_grounded = true;
    bool m_moving = false;
    bool m_shiftPressed = false;
    int m_jumpCount = 0;
    int m_evadeDirection = 1;

    int m_hp = 3;
    int m_maxHp = 3;
    int m_attackPower = 20;
    int m_attackMotion = 1;

    float m_frame = 0.0f;
    float m_evadeTimer = 0.0f;
    float m_evadeCooldownTimer = 0.0f;
    float m_invincibleTimer = 0.0f;
};
