#include "Stages.h"

#include "Constants.h"
#include "Geometry.h"
#include "Player.h"
#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace {
std::mt19937& Rng() {
    static std::mt19937 rng { std::random_device{}() };
    return rng;
}

float RandomRange(float minValue, float maxValue) {
    std::uniform_real_distribution<float> dist(minValue, maxValue);
    return dist(Rng());
}

int RandomInt(int minValue, int maxValue) {
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(Rng());
}

void DamagePlayerOnTouch(Player& player, const RectF& box) {
    if (Intersects(player.Bounds(), box)) {
        player.TakeDamage(1);
    }
}

void DrawBossHp(Renderer& renderer, int hp, int maxHp) {
    const float percent = maxHp > 0 ? std::clamp(static_cast<float>(hp) / static_cast<float>(maxHp), 0.0f, 1.0f) : 0.0f;
    constexpr float frameWidth = 512.0f;
    constexpr float barHeight = 16.0f;
    renderer.DrawImage(L"Assets\\bossui\\Enemy Health Frame.png", CanvasWidth * 0.5f, 24.0f, frameWidth, barHeight);
    renderer.DrawImageClip(L"Assets\\bossui\\EnemyHealthBar.png", 0.0f, 0.0f, 288.0f, 16.0f,
        CanvasWidth * 0.5f - (frameWidth - frameWidth * percent) * 0.5f, 24.0f, frameWidth * percent, barHeight);
}

struct Projectile {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float radius = 16.0f;
    float frame = 0.0f;
    float life = 0.0f;
};

class Ghost {
public:
    Ghost(float startX, float startY, int hp, float speed)
        : m_x(startX), m_y(startY), m_hp(hp), m_speed(speed), m_shape(RandomPattern(Rng(), true)) {}

    void Update(float deltaTime, Player& player) {
        if (m_dead) {
            return;
        }

        if (m_hitTimer > 0.0f) {
            m_hitFrame += 8.0f * deltaTime;
            if (m_hitFrame >= 3.0f) {
                m_hitTimer = 0.0f;
                m_hitFrame = 0.0f;
                if (m_hp > 0) {
                    m_shape = RandomPattern(Rng(), true);
                }
            }
        } else if (m_dieTimer > 0.0f) {
            m_dieFrame += 8.0f * deltaTime;
            if (m_dieFrame >= 3.0f) {
                m_dead = true;
            }
        } else {
            MoveToward(m_x, m_y, player.X(), player.Y(), m_speed, deltaTime);
        }

        if (m_hp <= 0 && m_dieTimer <= 0.0f) {
            m_dieTimer = 0.01f;
            m_shape = PatternId::Unknown;
        }

        if (m_dieTimer <= 0.0f) {
            DamagePlayerOnTouch(player, CenterBox(m_x, m_y, 59.0f, 76.0f));
            if (player.IsInvincible()) {
                m_dieTimer = 0.01f;
            }
        }
    }

    bool ApplyGesture(PatternId id, int damage) {
        if (m_dead || m_dieTimer > 0.0f || id != m_shape) {
            return false;
        }
        m_hp -= damage;
        m_hitTimer = 0.01f;
        m_hitFrame = 0.0f;
        return true;
    }

    void Draw(Renderer& renderer, const Player& player) const {
        if (m_dead) {
            return;
        }

        float left = 1981.0f;
        float bottom = 820.0f - 265.0f - 76.0f;
        float width = 59.0f;
        float height = 76.0f;
        if (m_dieTimer > 0.0f) {
            static constexpr float frames[3][4] = {
                {1143, 820 - 637 - 81, 69, 81}, {1242, 820 - 637 - 81, 71, 81}, {1349, 820 - 637 - 81, 53, 81}
            };
            const int index = std::clamp(static_cast<int>(m_dieFrame), 0, 2);
            left = frames[index][0];
            bottom = frames[index][1];
            width = frames[index][2];
            height = frames[index][3];
        } else if (m_hitTimer > 0.0f) {
            static constexpr float frames[4][4] = {
                {1175, 820 - 539 - 89, 101, 89}, {1289, 820 - 539 - 90, 100, 90},
                {1406, 820 - 554 - 80, 100, 80}, {1535, 820 - 554 - 80, 62, 80}
            };
            const int index = std::clamp(static_cast<int>(m_hitFrame), 0, 3);
            left = frames[index][0];
            bottom = frames[index][1];
            width = frames[index][2];
            height = frames[index][3];
        }

        const bool flip = player.X() >= m_x;
        renderer.DrawImageClip(L"Assets\\1stage\\level1-png-sprite.png", left, bottom, width, height, m_x, m_y, width, height, flip);
        if (m_dieTimer <= 0.0f) {
            DrawPattern(renderer, m_shape, m_x, m_y + 54.0f, 0.2f);
        }
    }

    bool IsDead() const { return m_dead; }

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    int m_hp = 20;
    float m_speed = 50.0f;
    PatternId m_shape = PatternId::Unknown;
    bool m_dead = false;
    float m_hitTimer = 0.0f;
    float m_hitFrame = 0.0f;
    float m_dieTimer = 0.0f;
    float m_dieFrame = 0.0f;
};

class Stage1Phase {
public:
    virtual ~Stage1Phase() = default;
    virtual void Update(float deltaTime, Player& player) = 0;
    virtual bool ApplyGesture(PatternId id, int damage) = 0;
    virtual void Draw(Renderer& renderer, const Player& player) const = 0;
    virtual bool Finished() const = 0;
};

class GhostWavePhase final : public Stage1Phase {
public:
    enum class Mode { LastOnly, FirstN, All };

    GhostWavePhase(const std::vector<int>& sides, Mode mode, int activeCount, int minHp, int maxHp, float speed)
        : m_mode(mode), m_activeCount(activeCount) {
        for (int side : sides) {
            const float x = side < 0 ? 0.0f : CanvasWidth + 50.0f;
            const float y = RandomRange(50.0f, CanvasHeight - 50.0f);
            m_ghosts.push_back(std::make_unique<Ghost>(x, y, RandomInt(minHp, maxHp), speed));
        }
    }

    void Update(float deltaTime, Player& player) override {
        if (m_ghosts.empty()) {
            return;
        }

        if (m_mode == Mode::LastOnly) {
            m_ghosts.back()->Update(deltaTime, player);
            if (m_ghosts.back()->IsDead()) {
                m_ghosts.pop_back();
            }
            return;
        }

        const int count = ActiveCount();
        for (int i = 0; i < count; ++i) {
            m_ghosts[static_cast<size_t>(i)]->Update(deltaTime, player);
        }
        for (int i = count - 1; i >= 0; --i) {
            if (m_ghosts[static_cast<size_t>(i)]->IsDead()) {
                m_ghosts.erase(m_ghosts.begin() + i);
            }
        }
    }

    bool ApplyGesture(PatternId id, int damage) override {
        if (m_ghosts.empty()) {
            return false;
        }

        if (m_mode == Mode::LastOnly) {
            return m_ghosts.back()->ApplyGesture(id, damage);
        }

        bool hit = false;
        const int count = ActiveCount();
        for (int i = 0; i < count; ++i) {
            hit = m_ghosts[static_cast<size_t>(i)]->ApplyGesture(id, damage) || hit;
        }
        return hit;
    }

    void Draw(Renderer& renderer, const Player& player) const override {
        if (m_mode == Mode::LastOnly) {
            if (!m_ghosts.empty()) {
                m_ghosts.back()->Draw(renderer, player);
            }
            return;
        }

        const int count = ActiveCount();
        for (int i = 0; i < count; ++i) {
            m_ghosts[static_cast<size_t>(i)]->Draw(renderer, player);
        }
    }

    bool Finished() const override { return m_ghosts.empty(); }

private:
    int ActiveCount() const {
        if (m_mode == Mode::All) {
            return static_cast<int>(m_ghosts.size());
        }
        return std::min(m_activeCount, static_cast<int>(m_ghosts.size()));
    }

private:
    Mode m_mode = Mode::All;
    int m_activeCount = 1;
    std::vector<std::unique_ptr<Ghost>> m_ghosts;
};

class GhostBoss {
public:
    void Reset() {
        m_x = CanvasWidth * 0.5f;
        m_y = CanvasHeight + 100.0f;
        m_hp = m_maxHp;
        m_state = Cutscene;
        m_timer = 0.0f;
        m_frame = 0.0f;
        m_hitFrame = 0.0f;
        m_dieFrame = 0.0f;
        m_hitFlash = 0.0f;
        m_die = false;
        m_cutscene = false;
        m_patternSpeed = 1.0f;
        m_prevPattern = 0;
        m_hitNum = 3;
        m_shape = RandomPattern(Rng(), false);
    }

    void Update(float deltaTime, Player& player) {
        if (m_die) {
            return;
        }

        m_frame = std::fmod(m_frame + 8.0f * deltaTime, 4.0f);
        m_timer += deltaTime;
        m_hitFlash = std::max(0.0f, m_hitFlash - deltaTime);
        if (m_hp <= 120) {
            m_patternSpeed = 2.0f;
        }
        if (m_hp <= 0 && m_state != Die) {
            m_state = Die;
            m_timer = 0.0f;
            m_dieFrame = 0.0f;
            m_shape = PatternId::Unknown;
        }

        switch (m_state) {
        case Cutscene:
            m_y -= 50.0f * deltaTime;
            if (m_timer >= 7.0f) {
                m_cutscene = true;
                m_state = Ready;
                m_timer = 0.0f;
            }
            break;
        case Ready:
            if (m_timer >= 1.0f) {
                BeginPattern0();
            }
            break;
        case Pattern0:
            UpdatePattern0(deltaTime);
            break;
        case Pattern1:
            UpdatePattern1(deltaTime, player);
            break;
        case Pattern2:
            MoveToward(m_x, m_y, player.X(), player.Y(), 50.0f, deltaTime);
            if (m_timer >= 8.0f || m_hitNum <= 0) {
                if (m_prevPattern == 0) {
                    m_prevPattern = 1;
                    BeginPattern1();
                } else {
                    m_prevPattern = 0;
                    BeginPattern0();
                }
            }
            break;
        case Hit:
            m_hitFrame += 8.0f * deltaTime;
            if (m_hitFrame >= 3.0f) {
                m_state = m_previousState;
                m_hitFrame = 0.0f;
                m_shape = RandomPattern(Rng(), false);
            }
            break;
        case Die:
            m_dieFrame += 2.0f * deltaTime;
            if (m_dieFrame >= 8.0f) {
                m_die = true;
            }
            break;
        }

        if (m_state != Die) {
            DamagePlayerOnTouch(player, CenterBox(m_x, m_y, 70.0f * 1.2f, 104.0f * 1.2f));
        }
    }

    bool ApplyGesture(PatternId id, int damage) {
        if (m_state != Pattern2 || m_hitNum <= 0 || id != m_shape || m_hp <= 0) {
            return false;
        }

        m_hp = std::max(0, m_hp - damage);
        --m_hitNum;
        m_hitFlash = 0.18f;
        m_previousState = m_state;
        m_state = Hit;
        m_hitFrame = 0.0f;
        return true;
    }

    void Draw(Renderer& renderer) const {
        if (m_die) {
            return;
        }

        float left = 3313.0f;
        float bottom = 820.0f - 709.0f - 104.0f;
        float width = 70.0f;
        float height = 104.0f;
        if (m_state == Die) {
            static constexpr float frames[8][4] = {
                {436, 820 - 566 - 106, 81, 106}, {530, 820 - 571 - 102, 82, 102},
                {629, 820 - 578 - 108, 86, 108}, {729, 820 - 581 - 111, 86, 111},
                {829, 820 - 591 - 105, 86, 105}, {1143, 820 - 637 - 81, 69, 81},
                {1242, 820 - 637 - 81, 71, 81}, {1349, 820 - 637 - 81, 53, 81}
            };
            const int index = std::clamp(static_cast<int>(m_dieFrame), 0, 7);
            left = frames[index][0]; bottom = frames[index][1]; width = frames[index][2]; height = frames[index][3];
        } else if (m_state == Hit) {
            static constexpr float frames[3][4] = {
                {2115, 820 - 443 - 105, 124, 105}, {2252, 820 - 450 - 102, 104, 102}, {2402, 820 - 450 - 97, 92, 97}
            };
            const int index = std::clamp(static_cast<int>(m_hitFrame), 0, 2);
            left = frames[index][0]; bottom = frames[index][1]; width = frames[index][2]; height = frames[index][3];
        } else if (m_state == Pattern0 && m_patternMoving) {
            left = 953.0f; bottom = 820.0f - 576.0f - 129.0f; width = 62.0f; height = 129.0f;
        } else if (m_state == Pattern1 && m_patternMoving) {
            static constexpr float frames[5][4] = {
                {282, 820 - 689 - 99, 74, 99}, {363, 820 - 689 - 97, 74, 97}, {444, 820 - 689 - 97, 73, 97},
                {525, 820 - 689 - 97, 76, 97}, {606, 820 - 699 - 97, 77, 97}
            };
            const int index = static_cast<int>(std::fmod(m_frame, 5.0f));
            left = frames[index][0]; bottom = frames[index][1]; width = frames[index][2]; height = frames[index][3];
        } else {
            static constexpr float frames[4][4] = {
                {3313, 820 - 709 - 104, 70, 104}, {3385, 820 - 709 - 104, 67, 104},
                {3457, 820 - 709 - 106, 66, 106}, {3529, 820 - 709 - 104, 66, 104}
            };
            const int index = static_cast<int>(m_frame) % 4;
            left = frames[index][0]; bottom = frames[index][1]; width = frames[index][2]; height = frames[index][3];
        }

        renderer.DrawImageClip(L"Assets\\1stage\\level1-png-sprite.png", left, bottom, width, height,
            m_x, m_y, width * 1.2f, height * 1.2f, true, m_hitFlash > 0.0f ? 0.45f : 1.0f);
        if (m_state != Die) {
            DrawBossHp(renderer, m_hp, m_maxHp);
        }
        if (m_cutscene && m_state == Pattern2 && m_hitNum > 0) {
            DrawPattern(renderer, m_shape, m_x, m_y + 88.0f, 0.2f);
        }
    }

    bool IsDead() const { return m_die; }

private:
    enum State { Cutscene, Ready, Pattern0, Pattern1, Pattern2, Hit, Die };

    void ResetForPattern(State next) {
        m_x = RandomRange(90.0f, CanvasWidth - 90.0f);
        m_y = CanvasHeight + 125.0f;
        m_timer = 0.0f;
        m_patternMoving = false;
        if (next == Pattern2) {
            const int remainder = m_hp % 60;
            if (remainder > 40 || remainder == 0) {
                m_hitNum = 3;
            } else if (remainder > 20) {
                m_hitNum = 2;
            } else {
                m_hitNum = 1;
            }
        }
        m_state = next;
    }

    void BeginPattern0() {
        ResetForPattern(Pattern0);
        m_targetX = 70.0f;
        m_targetY = 125.0f;
    }

    void BeginPattern1() {
        ResetForPattern(Pattern1);
        m_targetX = RandomRange(100.0f, CanvasWidth - 100.0f);
        m_targetY = CanvasHeight * 0.75f;
    }

    void UpdatePattern0(float deltaTime) {
        if (!m_patternMoving) {
            MoveToward(m_x, m_y, m_targetX, m_targetY, 1000.0f, deltaTime);
            if (std::abs(m_x - m_targetX) <= 5.0f && std::abs(m_y - m_targetY) <= 5.0f && m_timer >= 0.5f) {
                m_patternMoving = true;
            }
        } else {
            m_x += 50.0f * 6.0f * m_patternSpeed * deltaTime;
            if (m_x >= CanvasWidth + 50.0f) {
                ResetForPattern(Pattern2);
            }
        }
    }

    void UpdatePattern1(float deltaTime, const Player& player) {
        if (!m_patternMoving) {
            MoveToward(m_x, m_y, m_targetX, m_targetY, 1000.0f, deltaTime);
            if (std::abs(m_x - m_targetX) <= 5.0f && std::abs(m_y - m_targetY) <= 5.0f) {
                m_targetX = player.X();
                m_targetY = player.Y();
                if (m_timer >= 0.5f) {
                    m_patternMoving = true;
                }
            }
        } else {
            MoveToward(m_x, m_y, m_targetX, m_targetY, 50.0f * 6.0f * m_patternSpeed, deltaTime);
            if (std::abs(m_x - m_targetX) <= 6.0f && std::abs(m_y - m_targetY) <= 6.0f) {
                ResetForPattern(Pattern2);
            }
        }
    }

private:
    State m_state = Cutscene;
    State m_previousState = Ready;
    float m_x = 640.0f;
    float m_y = 820.0f;
    int m_maxHp = 240;
    int m_hp = 240;
    float m_timer = 0.0f;
    float m_frame = 0.0f;
    float m_hitFrame = 0.0f;
    float m_dieFrame = 0.0f;
    float m_hitFlash = 0.0f;
    float m_targetX = 0.0f;
    float m_targetY = 0.0f;
    float m_patternSpeed = 1.0f;
    int m_prevPattern = 0;
    int m_hitNum = 3;
    bool m_patternMoving = false;
    bool m_cutscene = false;
    bool m_die = false;
    PatternId m_shape = PatternId::Unknown;
};

class Stage1BossPhase final : public Stage1Phase {
public:
    Stage1BossPhase() { m_boss.Reset(); }
    void Update(float deltaTime, Player& player) override { m_boss.Update(deltaTime, player); }
    bool ApplyGesture(PatternId id, int damage) override { return m_boss.ApplyGesture(id, damage); }
    void Draw(Renderer& renderer, const Player&) const override { m_boss.Draw(renderer); }
    bool Finished() const override { return m_boss.IsDead(); }

private:
    GhostBoss m_boss;
};

class Stage1Controller final : public StageController {
public:
    void Reset() override {
        m_phaseIndex = 0;
        m_cleared = false;
        m_phases.clear();
        m_phases.push_back(std::make_unique<GhostWavePhase>(std::vector<int> { -1, -1, 1, 1, 1 }, GhostWavePhase::Mode::LastOnly, 1, 20, 20, 50.0f));
        m_phases.push_back(std::make_unique<GhostWavePhase>(std::vector<int> { -1, 1, -1, 1, -1, 1, -1, 1 }, GhostWavePhase::Mode::FirstN, 2, 20, 30, 50.0f));
        m_phases.push_back(std::make_unique<GhostWavePhase>(std::vector<int> { -1, 1, -1, 1, -1, 1, -1, 1, -1, 1 }, GhostWavePhase::Mode::All, 10, 10, 25, 50.0f));
        m_phases.push_back(std::make_unique<GhostWavePhase>(std::vector<int> { -1, 1, -1, 1, -1, 1, -1, 1, -1, 1 }, GhostWavePhase::Mode::LastOnly, 1, 20, 20, 150.0f));
        m_phases.push_back(std::make_unique<Stage1BossPhase>());
    }

    void Update(float deltaTime, Player& player) override {
        if (m_cleared || m_phaseIndex >= m_phases.size()) {
            return;
        }

        m_phases[m_phaseIndex]->Update(deltaTime, player);
        if (m_phases[m_phaseIndex]->Finished()) {
            ++m_phaseIndex;
            if (m_phaseIndex >= m_phases.size()) {
                m_cleared = true;
            }
        }
    }

    void ApplyGesture(PatternId id, Player& player) override {
        if (m_phaseIndex < m_phases.size() && m_phases[m_phaseIndex]->ApplyGesture(id, player.AttackPower())) {
            player.QueueSmash();
        }
    }

    void Draw(Renderer& renderer, const Player& player) const override {
        for (int i = 1; i <= 8; ++i) {
            renderer.DrawImage(L"Assets\\1stage\\" + std::to_wstring(i) + L".png",
                CanvasWidth * 0.5f, CanvasHeight * 0.5f, 1980.0f * 0.7f, 1080.0f * 0.7f);
        }
        if (m_phaseIndex < m_phases.size()) {
            m_phases[m_phaseIndex]->Draw(renderer, player);
        }
    }

    bool IsCleared() const override { return m_cleared; }

private:
    size_t m_phaseIndex = 0;
    bool m_cleared = false;
    std::vector<std::unique_ptr<Stage1Phase>> m_phases;
};

class KittyBoss {
public:
    void Reset() {
        m_started = false;
        m_hp = m_maxHp;
        m_x = CanvasWidth - 300.0f;
        m_y = CanvasHeight * 0.5f;
        m_dir = 1.0f;
        m_idleFrame = 0.0f;
        m_pattern = 0;
        m_timer = 0.0f;
        m_spawnTimer = 0.0f;
        m_shape = RandomPattern(Rng(), false);
        m_projectiles.clear();
        m_lasers.clear();
        m_littleKitties.clear();
    }

    void Start() {
        if (!m_started) {
            m_started = true;
            BeginPattern(0);
        }
    }

    void Update(float deltaTime, Player& player) {
        if (!m_started || IsDead()) {
            return;
        }

        m_idleFrame = std::fmod(m_idleFrame + 8.0f * deltaTime, 2.0f);
        m_y += 300.0f * deltaTime * m_dir;
        if (m_y >= CanvasHeight - 170.0f) {
            m_y = CanvasHeight - 170.0f;
            m_dir = -1.0f;
        } else if (m_y <= 170.0f) {
            m_y = 170.0f;
            m_dir = 1.0f;
        }

        m_timer += deltaTime;
        UpdatePattern(deltaTime, player);
        UpdateProjectiles(deltaTime, player);
    }

    bool ApplyGesture(PatternId id, int damage) {
        if (!m_started || id != m_shape || m_hp <= 0) {
            return false;
        }
        m_hp = std::max(0, m_hp - damage);
        m_hitFlash = 0.18f;
        m_shape = RandomPattern(Rng(), false);
        return true;
    }

    void Draw(Renderer& renderer) const {
        if (!m_started) {
            DrawPattern(renderer, PatternId::Heart, CanvasWidth * 0.5f, CanvasHeight - 110.0f, 0.6f);
            renderer.DrawTextPico(L"하트를 그리면 전투가 시작돼", 430.0f, 625.0f, 420.0f, 36.0f, 26.0f,
                D2D1::ColorF(D2D1::ColorF::White), DWRITE_TEXT_ALIGNMENT_CENTER);
            return;
        }

        const std::wstring texture = (static_cast<int>(m_idleFrame) == 0) ? L"Assets\\2stage\\boss1.png" : L"Assets\\2stage\\boss2.png";
        renderer.DrawImage(texture, m_x, m_y, 362.0f * 1.2f, 288.0f * 1.2f, m_hitFlash > 0.0f ? 0.45f : 1.0f);

        for (const Projectile& p : m_projectiles) {
            const int frame = static_cast<int>(std::fmod(p.frame, 28.0f)) + 1;
            renderer.DrawImage(L"Assets\\2stage\\attack_" + std::to_wstring(frame) + L".png", p.x, p.y, 40.0f, 40.0f);
        }
        for (const Projectile& laser : m_lasers) {
            const int frame = std::clamp(static_cast<int>(laser.frame), 0, 6) + 1;
            renderer.DrawImage(L"Assets\\2stage\\lightAttack" + std::to_wstring(std::min(frame, 4)) + L".png",
                CanvasWidth * 0.5f, laser.y, CanvasWidth, 42.0f, laser.life > 0.0f ? 0.85f : 0.35f);
        }
        for (const Projectile& kitty : m_littleKitties) {
            renderer.DrawImage(L"Assets\\2stage\\157.png", kitty.x, kitty.y, 70.0f, 70.0f);
        }

        DrawPattern(renderer, m_shape, m_x, m_y + 95.0f, 0.6f);
        DrawBossHp(renderer, m_hp, m_maxHp);
    }

    bool IsDead() const { return m_hp <= 0; }
    bool Started() const { return m_started; }

private:
    void BeginPattern(int pattern) {
        m_pattern = pattern;
        m_timer = 0.0f;
        m_spawnTimer = 0.0f;
        m_spawned = 0;
        m_projectiles.clear();
        m_lasers.clear();
        m_littleKitties.clear();
    }

    void UpdatePattern(float deltaTime, const Player& player) {
        if (m_pattern == 0) {
            if (m_projectiles.empty() && m_spawned < 8) {
                SpawnHoming(player);
                ++m_spawned;
            }
            if (m_spawned >= 8 && m_projectiles.empty()) {
                BeginPattern(1);
            }
        } else if (m_pattern == 1) {
            if (m_timer >= 2.0f && m_spawned < 3) {
                Projectile laser;
                laser.y = player.Y();
                laser.life = 1.0f;
                laser.frame = 0.0f;
                m_lasers.push_back(laser);
                ++m_spawned;
                m_timer = 0.0f;
            }
            if (m_spawned >= 3 && m_lasers.empty()) {
                BeginPattern(2);
            }
        } else if (m_pattern == 2) {
            m_spawnTimer += deltaTime;
            if (m_spawned < 8 && m_spawnTimer >= 0.5f) {
                Projectile kitty;
                kitty.x = RandomRange(160.0f, CanvasWidth - 160.0f);
                kitty.y = CanvasHeight + 50.0f;
                kitty.life = 0.0f;
                m_littleKitties.push_back(kitty);
                ++m_spawned;
                m_spawnTimer = 0.0f;
            }
            if (m_spawned >= 8 && m_littleKitties.empty()) {
                BeginPattern(3);
            }
        } else {
            m_spawnTimer += deltaTime;
            if (m_timer < 8.0f && m_spawnTimer >= 0.05f) {
                const float angle = RandomRange(135.0f, 225.0f) * 3.14159265f / 180.0f;
                Projectile p;
                p.x = m_x;
                p.y = m_y;
                p.vx = std::cos(angle) * 200.0f;
                p.vy = std::sin(angle) * 200.0f;
                p.radius = 15.0f;
                m_projectiles.push_back(p);
                m_spawnTimer = 0.0f;
            }
            if (m_timer >= 8.0f && m_projectiles.empty()) {
                BeginPattern(0);
            }
        }
    }

    void SpawnHoming(const Player& player) {
        Projectile p;
        p.x = m_x;
        p.y = m_y;
        const float d = std::max(1.0f, Distance(p.x, p.y, player.X(), player.Y()));
        p.vx = (player.X() - p.x) / d * 1200.0f;
        p.vy = (player.Y() - p.y) / d * 1200.0f;
        p.radius = 16.0f;
        m_projectiles.push_back(p);
    }

    void UpdateProjectiles(float deltaTime, Player& player) {
        m_hitFlash = std::max(0.0f, m_hitFlash - deltaTime);

        for (Projectile& p : m_projectiles) {
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.frame = std::fmod(p.frame + 40.0f * deltaTime, 28.0f);
            if (CircleIntersects(player.Bounds(), p.x, p.y, p.radius)) {
                player.TakeDamage(1);
                p.life = -999.0f;
            }
        }
        m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), [](const Projectile& p) {
            return p.life < -100.0f || p.x < -80.0f || p.x > CanvasWidth + 80.0f || p.y < -80.0f || p.y > CanvasHeight + 80.0f;
        }), m_projectiles.end());

        for (Projectile& laser : m_lasers) {
            laser.life -= deltaTime;
            laser.frame += 10.0f * deltaTime;
            if (std::abs(player.Y() - laser.y) < 28.0f) {
                player.TakeDamage(1);
            }
        }
        m_lasers.erase(std::remove_if(m_lasers.begin(), m_lasers.end(), [](const Projectile& p) { return p.life <= 0.0f; }), m_lasers.end());

        for (Projectile& kitty : m_littleKitties) {
            kitty.life += deltaTime;
            kitty.y -= 150.0f * deltaTime;
            kitty.x += std::sin(kitty.life * 3.0f) * 50.0f * deltaTime;
            if (Intersects(player.Bounds(), CenterBox(kitty.x, kitty.y, 50.0f, 50.0f))) {
                player.TakeDamage(1);
                kitty.life = -999.0f;
            }
        }
        m_littleKitties.erase(std::remove_if(m_littleKitties.begin(), m_littleKitties.end(), [](const Projectile& p) {
            return p.life < -100.0f || p.y < -80.0f;
        }), m_littleKitties.end());
    }

private:
    bool m_started = false;
    int m_maxHp = 360;
    int m_hp = 360;
    float m_x = 980.0f;
    float m_y = 360.0f;
    float m_dir = 1.0f;
    float m_idleFrame = 0.0f;
    float m_timer = 0.0f;
    float m_spawnTimer = 0.0f;
    float m_hitFlash = 0.0f;
    int m_pattern = 0;
    int m_spawned = 0;
    PatternId m_shape = PatternId::Unknown;
    std::vector<Projectile> m_projectiles;
    std::vector<Projectile> m_lasers;
    std::vector<Projectile> m_littleKitties;
};

class Stage2Controller final : public StageController {
public:
    void Reset() override {
        m_cleared = false;
        m_scrollY = 0.0f;
        m_fadeTimer = 0.0f;
        m_boss.Reset();
    }

    void Update(float deltaTime, Player& player) override {
        m_fadeTimer += deltaTime;
        if (m_boss.Started()) {
            m_scrollY = std::fmod(m_scrollY + 2000.0f * deltaTime, 3072.0f);
        }
        m_boss.Update(deltaTime, player);
        if (m_boss.IsDead()) {
            m_cleared = true;
        }
    }

    void ApplyGesture(PatternId id, Player& player) override {
        if (!m_boss.Started()) {
            if (id == PatternId::Heart) {
                m_boss.Start();
            }
            return;
        }
        if (m_boss.ApplyGesture(id, player.AttackPower())) {
            player.QueueSmash();
        }
    }

    void Draw(Renderer& renderer, const Player&) const override {
        if (!m_boss.Started()) {
            const float progress = std::min(1.0f, m_fadeTimer / 4.0f);
            renderer.DrawImage(L"Assets\\2stage\\1.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f, 872.0f * 1.6f, 479.0f * 1.6f, 1.0f - progress * 0.35f);
            renderer.DrawImage(L"Assets\\2stage\\2.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f, 726.0f * 1.8f, 574.0f * 1.8f, progress);
        } else {
            renderer.DrawImage(L"Assets\\2stage\\3.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f + m_scrollY, 1280.0f * 1.5f, 2048.0f * 1.5f);
            renderer.DrawImage(L"Assets\\2stage\\3.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f + m_scrollY - 3072.0f, 1280.0f * 1.5f, 2048.0f * 1.5f);
        }

        renderer.DrawImage(L"Assets\\2stage\\floor_1.png", 80.0f, 24.0f, 274.0f, 63.0f);
        renderer.DrawImage(L"Assets\\2stage\\floor_1.png", 200.0f, 24.0f, 274.0f, 63.0f);
        for (int i = 0; i < 3; ++i) {
            renderer.DrawImage(L"Assets\\2stage\\floor_2.png", 80.0f, 170.0f + 170.0f * i, 72.0f, 38.0f);
        }
        m_boss.Draw(renderer);
    }

    bool IsCleared() const override { return m_cleared; }

private:
    KittyBoss m_boss;
    float m_scrollY = 0.0f;
    float m_fadeTimer = 0.0f;
    bool m_cleared = false;
};

class SihoBoss {
public:
    void Reset() {
        m_hp = m_maxHp;
        m_x = 1000.0f;
        m_y = 300.0f;
        m_timer = 0.0f;
        m_frame = 0.0f;
        m_pattern = 0;
        m_subState = 0;
        m_dead = false;
        m_appeared = false;
        m_phase = 0;
        m_shape = RandomPattern(Rng(), false);
        m_projectiles.clear();
        m_attacks.clear();
    }

    void Update(float deltaTime, Player& player) {
        if (m_dead) {
            return;
        }

        m_timer += deltaTime;
        m_frame += 8.0f * deltaTime;
        m_hitFlash = std::max(0.0f, m_hitFlash - deltaTime);
        if (!m_appeared) {
            if (m_timer >= 4.0f) {
                m_appeared = true;
                BeginPattern(1);
            }
            return;
        }

        if (m_hp <= 0 && m_pattern != 14) {
            BeginPattern(14);
        } else if (m_hp <= 300 && m_phase < 2 && m_pattern != 11 && m_pattern != 14) {
            m_phase = 2;
            BeginPattern(11);
        } else if (m_hp <= 550 && m_phase < 1 && m_pattern != 6 && m_pattern != 11 && m_pattern != 14) {
            m_phase = 1;
            BeginPattern(6);
        }

        UpdateCurrentPattern(deltaTime, player);
        UpdateAttacks(deltaTime, player);
    }

    bool ApplyGesture(PatternId id, int damage) {
        if (!m_appeared || m_pattern == 14 || id != m_shape || m_hp <= 0) {
            return false;
        }
        m_hp = std::max(0, m_hp - damage);
        m_hitFlash = 0.18f;
        m_shape = RandomPattern(Rng(), false);
        return true;
    }

    void Draw(Renderer& renderer) const {
        DrawTerrain(renderer);
        DrawAttacks(renderer);

        if (!m_appeared) {
            const int frame = std::clamp(static_cast<int>(m_frame), 1, 8);
            renderer.DrawImage(L"Assets\\3stage\\Appear" + std::to_wstring(frame) + L".png", m_x, m_y, 64.0f * 3.0f, 64.0f * 3.0f);
            return;
        }

        std::wstring texture = L"Assets\\3stage\\Fox Human Idle1.png";
        float width = 64.0f * 3.0f;
        float height = 64.0f * 3.0f;
        if (m_pattern == 14) {
            const int frame = std::clamp(static_cast<int>(m_frame), 1, 18);
            texture = L"Assets\\3stage\\Die" + std::to_wstring(frame) + L".png";
            width = 96.0f * 3.0f;
        } else if (m_phase >= 2) {
            const int frame = static_cast<int>(std::fmod(m_frame, 8.0f)) + 1;
            texture = L"Assets\\3stage\\Fox Burning Idle" + std::to_wstring(frame) + L".png";
            width = 96.0f * 3.0f;
        } else if (m_phase >= 1) {
            const int frame = static_cast<int>(std::fmod(m_frame, 8.0f)) + 1;
            texture = L"Assets\\3stage\\Idle" + std::to_wstring(frame) + L".png";
            width = 96.0f * 3.0f;
        } else if (static_cast<int>(m_frame) % 2 == 1) {
            texture = L"Assets\\3stage\\Fox Human Idle2.png";
        }

        renderer.DrawImage(texture, m_x, m_y, width, height, m_hitFlash > 0.0f ? 0.45f : 1.0f);
        if (m_pattern != 14) {
            DrawPattern(renderer, m_shape, m_x, m_y + 95.0f, 0.4f);
            DrawBossHp(renderer, m_hp, m_maxHp);
        }
    }

    bool IsDead() const { return m_dead; }

private:
    void BeginPattern(int pattern) {
        m_pattern = pattern;
        m_subState = 0;
        m_timer = 0.0f;
        m_frame = 0.0f;
        m_targetX = 0.0f;
        m_targetY = 0.0f;
        m_velocityY = 0.0f;
    }

    void BeginRandomCombatPattern() {
        if (m_phase == 0) {
            BeginPattern(RandomInt(2, 5));
        } else if (m_phase == 1) {
            BeginPattern(RandomInt(8, 10));
        } else {
            BeginPattern(13);
        }
    }

    void UpdateCurrentPattern(float deltaTime, Player& player) {
        if (m_pattern == 1 || m_pattern == 7 || m_pattern == 12) {
            if (m_timer >= 1.0f) {
                BeginRandomCombatPattern();
            }
        } else if (m_pattern == 2 || m_pattern == 8) {
            UpdateJumpPattern(deltaTime, player, m_phase == 1 ? 7 : 1);
        } else if (m_pattern == 3) {
            if (m_timer >= 0.45f && m_projectiles.size() < 6) {
                SpawnFireball(player);
                m_timer = 0.0f;
            }
            if (m_projectiles.empty() && m_frame > 32.0f) {
                BeginPattern(1);
            }
        } else if (m_pattern == 4) {
            if (m_subState == 0) {
                m_targetX = player.X();
                m_subState = 1;
            }
            MoveToward(m_x, m_y, m_targetX, 300.0f, 1000.0f, deltaTime);
            if (m_timer >= 0.5f) {
                m_attacks.push_back({ m_x, m_y, 0.0f, 0.0f, 80.0f, 0.30f });
                BeginPattern(1);
            }
        } else if (m_pattern == 5) {
            if (m_subState == 0) {
                m_targetX = player.X();
                m_subState = 1;
            }
            if (m_timer >= 1.0f) {
                m_attacks.push_back({ m_targetX, 300.0f, 0.0f, 0.0f, 80.0f, 0.50f });
                BeginPattern(1);
            }
        } else if (m_pattern == 6) {
            if (m_timer >= 3.0f) {
                BeginPattern(7);
            }
        } else if (m_pattern == 9) {
            if (m_timer >= 0.45f && m_attacks.size() < 4) {
                m_attacks.push_back({ player.X(), 300.0f, 0.0f, 0.0f, 65.0f, 0.25f });
                m_timer = 0.0f;
            }
            if (m_attacks.empty() && m_frame > 28.0f) {
                BeginPattern(7);
            }
        } else if (m_pattern == 10) {
            if (m_timer >= 0.5f && m_attacks.size() < 3) {
                const float direction = player.X() >= m_x ? 1.0f : -1.0f;
                m_attacks.push_back({ m_x + direction * (280.0f + 240.0f * static_cast<float>(m_attacks.size())), m_y, 0.0f, 0.0f, 85.0f, 0.35f });
                m_timer = 0.0f;
            }
            if (m_attacks.empty() && m_frame > 30.0f) {
                BeginPattern(7);
            }
        } else if (m_pattern == 11) {
            if (m_timer >= 3.0f) {
                BeginPattern(12);
            }
        } else if (m_pattern == 13) {
            const float direction = (m_subState % 2 == 0) ? 1.0f : -1.0f;
            if (m_subState == 0) {
                m_x = -500.0f;
                m_y = 300.0f;
                m_subState = 1;
            }
            m_x += direction * 1000.0f * deltaTime;
            DamagePlayerOnTouch(player, CenterBox(m_x, m_y, 192.0f, 64.0f));
            if ((direction > 0.0f && m_x > 2480.0f) || (direction < 0.0f && m_x < -500.0f)) {
                ++m_subState;
                if (m_subState >= 4) {
                    m_x = 1000.0f;
                    m_y = 300.0f;
                    BeginPattern(12);
                } else {
                    m_x = direction > 0.0f ? 2480.0f : -500.0f;
                    m_y = (m_subState % 2 == 0) ? 380.0f : 300.0f;
                }
            }
        } else if (m_pattern == 14) {
            if (m_timer >= 6.0f) {
                m_dead = true;
            }
        }
    }

    void UpdateJumpPattern(float deltaTime, Player& player, int returnPattern) {
        if (m_subState == 0) {
            m_targetX = player.X();
            m_velocityY = 900.0f;
            m_subState = 1;
        }
        const float dx = (m_targetX - m_x) / 1.0f;
        m_x += dx * deltaTime;
        m_velocityY -= 1800.0f * deltaTime;
        m_y += m_velocityY * deltaTime;
        if (m_y <= 300.0f && m_velocityY < 0.0f) {
            m_y = 300.0f;
            m_attacks.push_back({ m_x, 280.0f, 0.0f, 0.0f, 120.0f, 0.35f });
            BeginPattern(returnPattern);
        }
    }

    void SpawnFireball(const Player& player) {
        Projectile p;
        p.x = m_x + 50.0f;
        p.y = m_y + 100.0f;
        const float d = std::max(1.0f, Distance(p.x, p.y, player.X(), player.Y()));
        p.vx = (player.X() - p.x) / d * 600.0f;
        p.vy = (player.Y() - p.y) / d * 600.0f;
        p.radius = 20.0f;
        m_projectiles.push_back(p);
    }

    void UpdateAttacks(float deltaTime, Player& player) {
        for (Projectile& p : m_projectiles) {
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.frame = std::fmod(p.frame + 8.0f * deltaTime, 4.0f);
            if (CircleIntersects(player.Bounds(), p.x, p.y, p.radius)) {
                player.TakeDamage(1);
                p.life = -999.0f;
            }
        }
        m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), [](const Projectile& p) {
            return p.life < -100.0f || p.x < -80.0f || p.x > CanvasWidth + 80.0f || p.y < -80.0f || p.y > CanvasHeight + 80.0f;
        }), m_projectiles.end());

        for (Projectile& attack : m_attacks) {
            attack.life -= deltaTime;
            if (CircleIntersects(player.Bounds(), attack.x, attack.y, attack.radius)) {
                player.TakeDamage(1);
            }
        }
        m_attacks.erase(std::remove_if(m_attacks.begin(), m_attacks.end(), [](const Projectile& attack) {
            return attack.life <= 0.0f;
        }), m_attacks.end());
    }

    void DrawTerrain(Renderer& renderer) const {
        renderer.DrawImage(L"Assets\\3stage\\Fox BackGround 3.png", CanvasWidth * 0.5f, 410.0f, 800.0f * 3.0f, 480.0f * 3.0f);
        renderer.DrawImage(L"Assets\\3stage\\Fox BackGround 2.png", CanvasWidth * 0.5f, 410.0f, 800.0f * 3.0f, 480.0f * 3.0f);
        renderer.DrawImage(L"Assets\\3stage\\Fox BackGround 1.png", CanvasWidth * 0.5f, 410.0f, 800.0f * 2.0f, 480.0f * 2.0f);
        renderer.DrawImage(L"Assets\\3stage\\Fox Platform.png", CanvasWidth * 0.5f, 120.0f, 752.0f * 2.0f, 128.0f * 2.0f);
    }

    void DrawAttacks(Renderer& renderer) const {
        for (const Projectile& p : m_projectiles) {
            const int frame = static_cast<int>(p.frame) + 1;
            renderer.DrawImage(L"Assets\\3stage\\Fox Fire" + std::to_wstring(std::clamp(frame, 1, 4)) + L".png", p.x, p.y, 64.0f, 64.0f);
        }
        for (const Projectile& attack : m_attacks) {
            renderer.FillEllipse(attack.x, attack.y, attack.radius, attack.radius, D2D1::ColorF(1.0f, 0.2f, 0.05f), 0.35f);
        }
    }

private:
    int m_maxHp = 800;
    int m_hp = 800;
    int m_pattern = 0;
    int m_subState = 0;
    int m_phase = 0;
    float m_x = 1000.0f;
    float m_y = 300.0f;
    float m_targetX = 0.0f;
    float m_targetY = 0.0f;
    float m_velocityY = 0.0f;
    float m_timer = 0.0f;
    float m_frame = 0.0f;
    float m_hitFlash = 0.0f;
    bool m_appeared = false;
    bool m_dead = false;
    PatternId m_shape = PatternId::Unknown;
    std::vector<Projectile> m_projectiles;
    std::vector<Projectile> m_attacks;
};

class Stage3Controller final : public StageController {
public:
    void Reset() override {
        m_cleared = false;
        m_boss.Reset();
    }

    void Update(float deltaTime, Player& player) override {
        m_boss.Update(deltaTime, player);
        if (m_boss.IsDead()) {
            m_cleared = true;
        }
    }

    void ApplyGesture(PatternId id, Player& player) override {
        if (m_boss.ApplyGesture(id, player.AttackPower())) {
            player.QueueSmash();
        }
    }

    void Draw(Renderer& renderer, const Player&) const override { m_boss.Draw(renderer); }
    bool IsCleared() const override { return m_cleared; }

private:
    SihoBoss m_boss;
    bool m_cleared = false;
};
}

std::unique_ptr<StageController> CreateStageController(int stage) {
    if (stage == 1) {
        auto controller = std::make_unique<Stage1Controller>();
        controller->Reset();
        return controller;
    }
    if (stage == 2) {
        auto controller = std::make_unique<Stage2Controller>();
        controller->Reset();
        return controller;
    }

    auto controller = std::make_unique<Stage3Controller>();
    controller->Reset();
    return controller;
}
