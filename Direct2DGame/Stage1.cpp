#include "Stages.h"

#include "Constants.h"
#include "Geometry.h"
#include "Patterns.h"
#include "Player.h"
#include "Renderer.h"
#include "StageCommon.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace {
using namespace stages;

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

} // namespace

std::unique_ptr<StageController> CreateStage1Controller() {
    auto controller = std::make_unique<Stage1Controller>();
    controller->Reset();
    return controller;
}
