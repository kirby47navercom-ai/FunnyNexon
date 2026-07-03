#include "Player.h"

#include "Constants.h"
#include "Input.h"
#include "Renderer.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace {
constexpr float Gravity = 2000.0f;
constexpr float WalkSpeed = 200.0f;
constexpr float RunSpeed = 350.0f;
constexpr float EvadeSpeed = 500.0f;
constexpr float JumpPower = 600.0f;
constexpr float EvadeDuration = 0.30f;
constexpr float EvadeCooldown = 1.50f;
constexpr float DoubleTapTime = 0.28f;

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

const std::vector<SpriteFrame>& WalkFrames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 28, 64, 0, 0}, {31, 0, 30, 64, 0, 0}, {64, 0, 33, 64, 0, 0},
        {100, 0, 31, 66, 0, 0}, {134, 0, 29, 66, 0, 0}, {166, 0, 33, 64, 0, 0}
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

const std::vector<SpriteFrame>& DoubleJumpFrames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 44, 44, 0, 0}, {47, 5, 45, 34, 0, 0}, {95, 0, 33, 46, 0, 0},
        {131, 5, 42, 39, 0, 0}, {176, 5, 37, 47, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& EvadeFrames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 44, 64, 0, 0}, {47, 0, 54, 43, 0, 0}, {104, 0, 41, 43, 0, 0},
        {148, 0, 46, 35, 0, 0}, {197, 0, 50, 41, 0, 0}, {250, 0, 37, 50, 0, 0},
        {290, 0, 32, 56, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& Attack1Frames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 55, 63, 0, 0}, {58, 0, 38, 82, -5, 10}, {97, 0, 56, 75, -7, 9},
        {156, 0, 55, 79, -5, 10}, {214, 0, 77, 82, 14, 10}, {294, 0, 69, 58, 16, -2},
        {366, 0, 68, 61, 16, -2}, {437, 0, 52, 65, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& Attack2Frames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 52, 60, 0, 0}, {55, 0, 75, 49, 55, -12}, {133, 0, 70, 50, 55, -10},
        {206, 0, 60, 57, 45, -4}, {269, 0, 50, 62, 35, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& Attack3Frames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 45, 73, 0, 0}, {48, 0, 45, 75, 0, 0}, {96, 0, 46, 76, 0, 0},
        {145, 0, 46, 70, 0, 0}, {194, 0, 40, 65, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& Attack4Frames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 60, 64, -30, 0}, {63, 0, 60, 75, -4, 12}, {126, 0, 38, 75, 0, 13},
        {167, 0, 99, 60, 20, 0}, {269, 0, 74, 59, 45, -2}, {346, 0, 73, 60, 45, -2},
        {422, 0, 62, 63, 30, 0}, {487, 0, 52, 65, 20, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& Attack6Frames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 54, 61, 0, 0}, {57, 0, 56, 59, 0, 0}, {116, 0, 90, 51, 70, 0},
        {209, 0, 86, 53, 70, 0}, {298, 0, 86, 53, 70, 0}, {387, 0, 86, 53, 70, 0},
        {476, 0, 81, 57, 50, 0}, {560, 0, 52, 65, 10, 8}
    };
    return frames;
}

const std::vector<SpriteFrame>& HitFrames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 44, 65, 0, 0}, {47, 0, 40, 63, 0, 0}, {90, 0, 36, 63, 0, 0}, {129, 0, 31, 65, 0, 0}
    };
    return frames;
}

const std::vector<SpriteFrame>& DeadFrames() {
    static const std::vector<SpriteFrame> frames = {
        {0, 0, 39, 62, 0, 0}, {42, 9, 50, 56, 0, 0}, {95, 25, 71, 43, 0, 0},
        {169, 38, 67, 36, 0, 0}, {239, 35, 68, 45, 0, 0}, {310, 30, 60, 58, 0, 0},
        {373, 18, 60, 62, 0, 0}, {436, 0, 46, 38, 0, 0}, {485, 5, 67, 30, 0, 0},
        {555, 5, 65, 31, 0, 0}, {623, 0, 68, 28, 0, 0}, {694, 0, 68, 29, 0, 0},
        {765, 0, 68, 29, 0, 0}
    };
    return frames;
}

const SpriteFrame& PickFrame(const std::vector<SpriteFrame>& frames, float frame) {
    const int index = std::clamp(static_cast<int>(frame), 0, static_cast<int>(frames.size()) - 1);
    return frames[static_cast<size_t>(index)];
}

std::mt19937& Rng() {
    static std::mt19937 rng { std::random_device{}() };
    return rng;
}
}

void Player::Reset(const GameState& state, int stage) {
    m_ground = GroundForStage(stage);
    m_x = CanvasWidth * 0.5f;
    m_y = m_ground;
    m_velocityY = 0.0f;
    m_grounded = true;
    m_moving = false;
    m_facingLeft = false;
    m_shiftPressed = false;
    m_jumpCount = 0;
    m_evadeDirection = 1;
    m_lastATap = -10.0f;
    m_lastDTap = -10.0f;
    m_aliveTime = 0.0f;

    m_maxHp = state.playerMaxHp;
    m_hp = m_maxHp;
    m_attackPower = state.playerAttack;
    m_attackMotion = 1;
    m_frame = 0.0f;
    m_evadeTimer = 0.0f;
    m_evadeCooldownTimer = 0.0f;
    m_invincibleTimer = 0.0f;
    m_mode = Idle;
}

void Player::Update(const InputState& input, float deltaTime, bool gestureOpen) {
    m_aliveTime += deltaTime;

    if (m_invincibleTimer > 0.0f) {
        m_invincibleTimer = std::max(0.0f, m_invincibleTimer - deltaTime);
    }
    if (m_evadeCooldownTimer > 0.0f) {
        m_evadeCooldownTimer = std::max(0.0f, m_evadeCooldownTimer - deltaTime);
    }

    if (m_mode == Dead) {
        m_frame = std::min(m_frame + 8.0f * deltaTime / 3.0f, static_cast<float>(DeadFrames().size() - 1));
        m_velocityY -= Gravity * deltaTime;
        m_y = std::max(m_ground, m_y + m_velocityY * deltaTime);
        return;
    }

    if (m_mode == Hit) {
        m_frame += 8.0f * deltaTime;
        if (m_frame >= static_cast<float>(HitFrames().size())) {
            ChangeMode(Idle);
        }
        return;
    }

    if (m_mode == Attack) {
        const std::vector<SpriteFrame>* frames = &Attack1Frames();
        if (m_attackMotion == 2) {
            frames = &Attack2Frames();
        } else if (m_attackMotion == 3) {
            frames = &Attack3Frames();
        } else if (m_attackMotion == 4) {
            frames = &Attack4Frames();
        } else if (m_attackMotion == 6) {
            frames = &Attack6Frames();
        }

        m_frame += 12.0f * deltaTime;
        if (m_frame >= static_cast<float>(frames->size())) {
            ChangeMode(Idle);
        }
        return;
    }

    if (m_mode == Evade) {
        m_evadeTimer -= deltaTime;
        ApplyHorizontalMove(static_cast<float>(m_evadeDirection), EvadeSpeed, deltaTime);
        m_frame = std::fmod(m_frame + 16.0f * deltaTime, static_cast<float>(EvadeFrames().size()));
        if (m_evadeTimer <= 0.0f) {
            ChangeMode(input.IsKeyDown('A') || input.IsKeyDown('D') ? Walk : Idle);
        }
        return;
    }

    if (gestureOpen) {
        m_moving = false;
        if (m_grounded && m_mode != Idle) {
            ChangeMode(Idle);
        }
        m_frame = std::fmod(m_frame + 8.0f * deltaTime, static_cast<float>(IdleFrames().size()));
        return;
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
    m_shiftPressed = input.IsKeyDown(VK_SHIFT);

    if (input.WasKeyPressed('A')) {
        if (m_aliveTime - m_lastATap <= DoubleTapTime && m_evadeCooldownTimer <= 0.0f) {
            m_evadeDirection = -1;
            m_facingLeft = true;
            ChangeMode(Evade);
            return;
        }
        m_lastATap = m_aliveTime;
    }
    if (input.WasKeyPressed('D')) {
        if (m_aliveTime - m_lastDTap <= DoubleTapTime && m_evadeCooldownTimer <= 0.0f) {
            m_evadeDirection = 1;
            m_facingLeft = false;
            ChangeMode(Evade);
            return;
        }
        m_lastDTap = m_aliveTime;
    }

    ApplyHorizontalMove(direction, m_shiftPressed ? RunSpeed : WalkSpeed, deltaTime);

    if (input.WasKeyPressed(VK_SPACE) && m_jumpCount < 2) {
        m_velocityY = JumpPower;
        m_grounded = false;
        ++m_jumpCount;
        ChangeMode(m_jumpCount == 1 ? Jump : DoubleJump);
    }

    m_velocityY -= Gravity * deltaTime;
    m_y += m_velocityY * deltaTime;
    if (m_y <= m_ground) {
        m_y = m_ground;
        m_velocityY = 0.0f;
        m_grounded = true;
        m_jumpCount = 0;
    }

    if (!m_grounded) {
        if (m_mode != Jump && m_mode != DoubleJump) {
            ChangeMode(Jump);
        }
        const std::vector<SpriteFrame>& frames = (m_mode == DoubleJump) ? DoubleJumpFrames() : JumpFrames();
        m_frame = std::fmod(m_frame + 16.0f * deltaTime, static_cast<float>(frames.size()));
    } else if (m_moving) {
        ChangeMode(m_shiftPressed ? Run : Walk);
        const std::vector<SpriteFrame>& frames = m_shiftPressed ? RunFrames() : WalkFrames();
        m_frame = std::fmod(m_frame + (m_shiftPressed ? 12.0f : 8.0f) * deltaTime, static_cast<float>(frames.size()));
    } else {
        if (m_mode != Idle) {
            ChangeMode(Idle);
        }
        m_frame = std::fmod(m_frame + 8.0f * deltaTime, static_cast<float>(IdleFrames().size()));
    }
}

void Player::Draw(Renderer& renderer) const {
    if (m_invincibleTimer > 0.0f && (static_cast<int>(m_invincibleTimer * 20.0f) % 2) == 0) {
        return;
    }

    std::wstring texture = L"Assets\\Ramona\\Ramona_idle.png";
    const std::vector<SpriteFrame>* frames = &IdleFrames();
    if (m_mode == Dead) {
        texture = L"Assets\\Ramona\\Ramona_dead.png";
        frames = &DeadFrames();
    } else if (m_mode == Hit) {
        texture = L"Assets\\Ramona\\Ramona_hit.png";
        frames = &HitFrames();
    } else if (m_mode == Attack) {
        texture = L"Assets\\Ramona\\Ramona_action" + std::to_wstring(m_attackMotion) + L".png";
        if (m_attackMotion == 2) {
            frames = &Attack2Frames();
        } else if (m_attackMotion == 3) {
            frames = &Attack3Frames();
        } else if (m_attackMotion == 4) {
            frames = &Attack4Frames();
        } else if (m_attackMotion == 6) {
            frames = &Attack6Frames();
        } else {
            frames = &Attack1Frames();
        }
    } else if (m_mode == Evade) {
        texture = L"Assets\\Ramona\\Ramona_evade.png";
        frames = &EvadeFrames();
    } else if (m_mode == DoubleJump) {
        texture = L"Assets\\Ramona\\Ramona_double_jump.png";
        frames = &DoubleJumpFrames();
    } else if (m_mode == Jump) {
        texture = L"Assets\\Ramona\\Ramona_jump.png";
        frames = &JumpFrames();
    } else if (m_mode == Run) {
        texture = L"Assets\\Ramona\\Ramona_run.png";
        frames = &RunFrames();
    } else if (m_mode == Walk) {
        texture = L"Assets\\Ramona\\Ramona_walk.png";
        frames = &WalkFrames();
    }

    const SpriteFrame& sprite = PickFrame(*frames, m_frame);
    renderer.DrawImageClip(texture, sprite.left, sprite.bottom, sprite.width, sprite.height,
        m_x + (m_facingLeft ? -sprite.offsetX : sprite.offsetX), m_y + sprite.offsetY,
        sprite.width, sprite.height, m_facingLeft);
}

void Player::QueueSmash() {
    if (m_mode == Hit || m_mode == Dead || m_mode == Evade) {
        return;
    }

    std::uniform_int_distribution<int> pick(1, 5);
    m_attackMotion = pick(Rng());
    if (m_attackMotion == 5) {
        m_attackMotion = 6;
    }
    ChangeMode(Attack);
}

bool Player::TakeDamage(int amount) {
    if (m_invincibleTimer > 0.0f || IsRollInvincible() || m_hp <= 0) {
        return false;
    }

    m_hp = std::max(0, m_hp - amount);
    m_invincibleTimer = 2.0f;
    ChangeMode(m_hp <= 0 ? Dead : Hit);
    return true;
}

HitBox Player::Bounds() const {
    return { m_x - 18.0f, m_y - 32.0f, m_x + 18.0f, m_y + 32.0f };
}

bool Player::IsRollInvincible() const {
    return m_mode == Evade && m_evadeTimer > 0.0f;
}

float Player::GroundForStage(int) const {
    return 100.0f;
}

void Player::ChangeMode(Mode mode) {
    if (m_mode == mode) {
        return;
    }

    m_mode = mode;
    m_frame = 0.0f;
    if (m_mode == Evade) {
        m_evadeTimer = EvadeDuration;
        m_evadeCooldownTimer = EvadeCooldown;
    }
}

void Player::ApplyHorizontalMove(float direction, float speed, float deltaTime) {
    m_x = std::clamp(m_x + direction * speed * deltaTime, 25.0f, CanvasWidth - 25.0f);
}
