#include "Player.h"

#include "Constants.h"
#include "Input.h"
#include "Renderer.h"

#include <Windows.h>
#include <algorithm>
#include <vector>

namespace {
constexpr float Gravity = 2000.0f;
constexpr float WalkSpeed = 200.0f;
constexpr float RunSpeed = 350.0f;
constexpr float JumpPower = 620.0f;

struct SpriteFrame {
    float left;
    float bottom;
    float width;
    float height;
    float offsetX;
    float offsetY;
};

const std::vector<SpriteFrame>& IdleFrames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 30, 64, 0, 0}, {33, 0, 30, 64, 0, 0}, {66, 0, 29, 64, 0, 0},
        {99, 0, 30, 64, 0, 0}, {131, 0, 31, 64, 0, 0}, {165, 0, 31, 64, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& RunFrames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 47, 59, 0, 0}, {50, 0, 42, 65, 0, 0}, {95, 5, 47, 59, 0, 0},
        {145, 3, 45, 59, 0, 0}, {193, 0, 46, 59, 0, 0}, {242, 0, 40, 65, 0, 0},
        {285, 5, 46, 59, 0, 0}, {334, 0, 44, 61, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& JumpFrames() {
    static const std::vector<SpriteFrame> frames = {
        {1, 0, 41, 51, 0, 0}, {45, 12, 36, 65, 0, 0}, {84, 14, 41, 61, 0, 0},
        {128, 18, 44, 58, 0, 0}, {175, 20, 45, 53, 0, 0}, {223, 18, 46, 56, 0, 0},
        {272, 14, 47, 62, 0, 0}, {322, 7, 45, 69, 0, 0}, {370, 0, 42, 52, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& AttackFrames() {
    // 원본 Ramona_action1 좌표. 다른 공격 모션은 같은 방식으로 추가하면 된다.
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 55, 63, 0, 0}, {58, 0, 38, 82, -5, 10}, {97, 0, 56, 75, -7, 9},
        {156, 0, 55, 79, -5, 10}, {214, 0, 77, 82, 14, 10}, {294, 0, 69, 58, 16, -2},
        {366, 0, 68, 61, 16, -2}, {437, 0, 52, 65, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& HitFrames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 44, 65, 0, 0}, {47, 0, 40, 63, 0, 0}, {90, 0, 36, 63, 0, 0}, {129, 0, 31, 65, 0, 0}
    };
    return frames;
}

const SpriteFrame& PickFrame(const std::vector<SpriteFrame>& frames, float frame) {
    const int index = std::clamp(static_cast<int>(frame), 0, static_cast<int>(frames.size()) - 1);
    return frames[index];
}
}

void Player::Reset(const GameState& state, int stage) {
    m_ground = GroundForStage(stage);
    m_x = (stage == 2) ? 80.0f : CanvasWidth * 0.5f;
    m_y = (stage == 3) ? 260.0f : m_ground;
    m_velocityY = 0.0f;
    m_grounded = true;
    m_moving = false;
    m_facingLeft = false;

    m_maxHp = state.playerMaxHp;
    m_hp = m_maxHp;
    m_attackPower = state.playerAttack;
    m_frame = 0.0f;
    m_attackTimer = 0.0f;
    m_invincibleTimer = 0.0f;
}

void Player::Update(const InputState& input, float deltaTime) {
    if (m_invincibleTimer > 0.0f) {
        m_invincibleTimer -= deltaTime;
    }

    float direction = 0.0f;
    if (input.IsKeyDown('A')) {
        direction -= 1.0f;
    }
    if (input.IsKeyDown('D')) {
        direction += 1.0f;
    }

    if (direction < 0.0f) {
        m_facingLeft = true;
    } else if (direction > 0.0f) {
        m_facingLeft = false;
    }
    m_moving = direction != 0.0f;

    const float speed = input.IsKeyDown(VK_SHIFT) ? RunSpeed : WalkSpeed;
    m_x = std::clamp(m_x + direction * speed * deltaTime, 25.0f, CanvasWidth - 25.0f);

    if (input.WasKeyPressed(VK_SPACE) && m_grounded) {
        m_velocityY = JumpPower;
        m_grounded = false;
    }

    m_velocityY -= Gravity * deltaTime;
    m_y += m_velocityY * deltaTime;
    if (m_y <= m_ground) {
        m_y = m_ground;
        m_velocityY = 0.0f;
        m_grounded = true;
    }

    if (m_attackTimer > 0.0f) {
        m_attackTimer -= deltaTime;
        m_frame += 16.0f * deltaTime;
    } else if (!m_grounded) {
        m_frame += 10.0f * deltaTime;
    } else if (m_moving) {
        m_frame += 10.0f * deltaTime;
    } else {
        m_frame += 6.0f * deltaTime;
    }
}

void Player::Draw(Renderer& renderer) const {
    if (m_invincibleTimer > 0.0f && (static_cast<int>(m_invincibleTimer * 20.0f) % 2) == 0) {
        return;
    }

    std::wstring texture = L"Assets\\Ramona\\Ramona_idle.png";
    const std::vector<SpriteFrame>* frames = &IdleFrames();
    float frame = m_frame;

    if (m_invincibleTimer > 0.0f) {
        texture = L"Assets\\Ramona\\Ramona_hit.png";
        frames = &HitFrames();
    } else if (m_attackTimer > 0.0f) {
        texture = L"Assets\\Ramona\\Ramona_action1.png";
        frames = &AttackFrames();
        const float progress = 1.0f - std::clamp(m_attackTimer / 0.45f, 0.0f, 1.0f);
        frame = progress * static_cast<float>(frames->size());
    } else if (!m_grounded) {
        texture = L"Assets\\Ramona\\Ramona_jump.png";
        frames = &JumpFrames();
    } else if (m_moving) {
        texture = L"Assets\\Ramona\\Ramona_run.png";
        frames = &RunFrames();
    }

    const SpriteFrame& sprite = PickFrame(*frames, frame);
    renderer.DrawImageClip(texture, sprite.left, sprite.bottom, sprite.width, sprite.height,
        m_x + (m_facingLeft ? -sprite.offsetX : sprite.offsetX), m_y + sprite.offsetY,
        sprite.width, sprite.height, m_facingLeft);
}

void Player::StartAttack() {
    m_attackTimer = 0.45f;
    m_frame = 0.0f;
}

bool Player::TakeDamage(int amount) {
    if (m_invincibleTimer > 0.0f || m_hp <= 0) {
        return false;
    }

    m_hp = std::max(0, m_hp - amount);
    m_invincibleTimer = 1.2f;
    m_frame = 0.0f;
    return true;
}

HitBox Player::Bounds() const {
    return { m_x - 18.0f, m_y - 32.0f, m_x + 18.0f, m_y + 32.0f };
}

float Player::GroundForStage(int stage) const {
    if (stage == 2) {
        return 70.0f;
    }
    if (stage == 3) {
        return 25.0f;
    }
    return 100.0f;
}

