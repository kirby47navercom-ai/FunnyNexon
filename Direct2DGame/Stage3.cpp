#include "Stages.h"

#include "Constants.h"
#include "Geometry.h"
#include "Patterns.h"
#include "Player.h"
#include "Renderer.h"
#include "StageCommon.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace {
using namespace stages;

constexpr float Pi = 3.14159265f;
constexpr float BossGroundY = 300.0f;

enum class TerrainPattern {
    Vine = 1,
    Water = 2,
    Flame = 3
};

enum class VineMode {
    Appear,
    Hold,
    Disappear
};

class Stage3Terrain {
public:
    void Reset() {
        m_order = { TerrainPattern::Vine, TerrainPattern::Water, TerrainPattern::Flame };
        std::shuffle(m_order.begin(), m_order.end(), Rng());
        m_current = 0;
        m_transition = false;
        m_transitionTimer = 0.0f;

        m_vineMode = VineMode::Appear;
        m_vineFrame = 0.0f;
        m_vineDisappearFrame = 0.0f;
        m_vineDuration = 0.0f;
        m_vineX = static_cast<float>(RandomInt(558, 1422));

        m_waterFrame = 0.0f;
        m_waterX = -200.0f;
        m_waterDirection = 1.0f;

        m_flameFrame = 0.0f;
        m_flameBallFrame = 0.0f;
        m_flameBallX = -100.0f;
        m_flameBallY = 1050.0f;
        m_flameBallSpeed = 800.0f;
    }

    void AdvancePattern() {
        m_current = (m_current + 1) % m_order.size();
        m_transition = true;
        m_transitionTimer = 0.0f;
    }

    void Update(float deltaTime, Player& player) {
        if (m_transition) {
            m_transitionTimer += deltaTime;
            if (m_transitionTimer >= 0.8f) {
                m_transition = false;
                m_transitionTimer = 0.0f;
            }
        }

        switch (m_order[m_current]) {
        case TerrainPattern::Vine:
            UpdateVine(deltaTime, player);
            break;
        case TerrainPattern::Water:
            UpdateWater(deltaTime, player);
            break;
        case TerrainPattern::Flame:
            UpdateFlame(deltaTime, player);
            break;
        }
    }

    void Draw(Renderer& renderer, bool hazardsEnabled) const {
        DrawStage3Backdrop(renderer);
        if (!hazardsEnabled) {
            return;
        }

        switch (m_order[m_current]) {
        case TerrainPattern::Vine:
            DrawVine(renderer);
            break;
        case TerrainPattern::Water:
            DrawWater(renderer);
            break;
        case TerrainPattern::Flame:
            DrawFlame(renderer);
            break;
        }

        if (m_transition) {
            const float half = 0.4f;
            const float alpha = m_transitionTimer < half
                ? m_transitionTimer / half
                : 1.0f - (m_transitionTimer - half) / half;
            renderer.DrawImage(L"Assets\\3stage\\White.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f,
                CanvasWidth, CanvasHeight, std::clamp(alpha, 0.0f, 0.85f));
        }
    }

private:
    void UpdateVine(float deltaTime, Player& player) {
        if (m_vineMode == VineMode::Appear) {
            m_vineFrame += 8.0f * deltaTime;
            if (m_vineFrame >= 11.0f) {
                m_vineFrame = 11.0f;
                m_vineMode = VineMode::Hold;
                m_vineDuration = 0.0f;
            }
        } else if (m_vineMode == VineMode::Hold) {
            m_vineDuration += deltaTime;
            DamagePlayerOnTouch(player, VineBox());
            if (m_vineDuration >= 4.0f) {
                m_vineMode = VineMode::Disappear;
                m_vineDisappearFrame = 0.0f;
            }
        } else {
            m_vineDisappearFrame += 8.0f * deltaTime;
            if (m_vineDisappearFrame >= 11.0f) {
                m_vineMode = VineMode::Appear;
                m_vineFrame = 0.0f;
                m_vineDisappearFrame = 0.0f;
                m_vineX = static_cast<float>(RandomInt(558, 1422));
            }
        }
    }

    void UpdateWater(float deltaTime, Player& player) {
        m_waterFrame = std::fmod(m_waterFrame + 8.0f * deltaTime, 8.0f);
        m_waterX += 400.0f * deltaTime * m_waterDirection;
        if (m_waterX > 2780.0f) {
            m_waterDirection = -1.0f;
        } else if (m_waterX < -800.0f) {
            m_waterDirection = 1.0f;
        }

        for (const RectF& box : WaterBoxes()) {
            DamagePlayerOnTouch(player, box);
        }
    }

    void UpdateFlame(float deltaTime, Player& player) {
        m_flameFrame = std::fmod(m_flameFrame + 8.0f * deltaTime, 5.0f);
        m_flameBallFrame = std::fmod(m_flameBallFrame + 8.0f * deltaTime, 5.0f);
        m_flameBallY += m_flameBallSpeed * deltaTime;

        if (m_flameBallY > 900.0f) {
            m_flameBallX = player.X();
            m_flameBallY = -50.0f;
            m_flameBallSpeed = 200.0f;
        }

        DamagePlayerOnTouch(player, CenterBox(m_flameBallX, m_flameBallY + 34.0f, 68.0f, 60.0f));
        DamagePlayerOnTouch(player, CenterBox(m_flameBallX, m_flameBallY + 39.0f, 38.0f, 70.0f));
    }

    RectF VineBox() const {
        return { m_vineX - 88.0f, 140.0f - 176.0f, m_vineX + 88.0f, 140.0f + 176.0f };
    }

    std::array<RectF, 4> WaterBoxes() const {
        const float x = m_waterX - (m_waterDirection < 0.0f ? 130.0f : 0.0f);
        return {
            RectF { x - 20.0f, 260.0f - 40.0f, x + 150.0f, 260.0f + 20.0f },
            RectF { x + 70.0f, 260.0f - 50.0f, x + 170.0f, 260.0f },
            RectF { x + 10.0f, 260.0f - 40.0f, x + 120.0f, 260.0f + 40.0f },
            RectF { x - 40.0f, 260.0f - 160.0f, x + 60.0f, 260.0f }
        };
    }

    void DrawVine(Renderer& renderer) const {
        renderer.DrawImage(L"Assets\\3stage\\Fox Vine1.png", m_vineX, 140.0f, 128.0f * 2.0f, 176.0f * 2.0f, 0.65f);
        if (m_vineMode == VineMode::Appear || m_vineMode == VineMode::Hold) {
            const int frame = std::clamp(static_cast<int>(m_vineFrame) + 1, 1, 12);
            renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Vine Needle Appear", frame), m_vineX, 140.0f,
                128.0f * 2.0f, 176.0f * 2.0f);
        } else {
            const int frame = std::clamp(static_cast<int>(m_vineDisappearFrame) + 1, 1, 12);
            renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Vine Needle Disappear", frame), m_vineX, 140.0f,
                128.0f * 2.0f, 176.0f * 2.0f);
        }
    }

    void DrawWater(Renderer& renderer) const {
        const int waterFrame = static_cast<int>(m_waterFrame) + 1;
        renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Water", waterFrame), CanvasWidth * 0.5f, 42.0f,
            CanvasWidth, 160.0f, 0.85f);

        const int waveFrame = static_cast<int>(m_waterFrame) + 1;
        const float drawX = m_waterX - (m_waterDirection < 0.0f ? 130.0f : 0.0f);
        renderer.DrawImageClip(NumberedAsset(L"Assets\\3stage\\Fox Wave", waveFrame), 0.0f, 0.0f, 224.0f, 160.0f,
            drawX, 260.0f, 224.0f * 2.0f, 160.0f * 2.0f, m_waterDirection < 0.0f);
    }

    void DrawFlame(Renderer& renderer) const {
        const int flameFrame = static_cast<int>(m_flameFrame) + 1;
        renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Flame", flameFrame), CanvasWidth * 0.5f, CanvasHeight * 0.5f,
            CanvasWidth, CanvasHeight, 0.55f);

        const int ballFrame = static_cast<int>(m_flameBallFrame) + 1;
        renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Flame Ball", ballFrame), m_flameBallX, m_flameBallY,
            64.0f * 2.0f, 80.0f * 2.0f);
    }

private:
    std::array<TerrainPattern, 3> m_order { TerrainPattern::Vine, TerrainPattern::Water, TerrainPattern::Flame };
    size_t m_current = 0;
    bool m_transition = false;
    float m_transitionTimer = 0.0f;

    VineMode m_vineMode = VineMode::Appear;
    float m_vineFrame = 0.0f;
    float m_vineDisappearFrame = 0.0f;
    float m_vineDuration = 0.0f;
    float m_vineX = 900.0f;

    float m_waterFrame = 0.0f;
    float m_waterX = -200.0f;
    float m_waterDirection = 1.0f;

    float m_flameFrame = 0.0f;
    float m_flameBallFrame = 0.0f;
    float m_flameBallX = -100.0f;
    float m_flameBallY = 1050.0f;
    float m_flameBallSpeed = 800.0f;
};

struct RushFire {
    float x = 0.0f;
    float y = 0.0f;
    float frame = 0.0f;
    int direction = 0;
    bool active = false;
    bool released = false;
};

class SihoBoss {
public:
    void Reset() {
        m_hp = m_maxHp;
        m_pattern = 0;
        m_subState = 0;
        m_phase = 0;
        m_x = 1000.0f;
        m_y = BossGroundY;
        m_targetX = 0.0f;
        m_velocityX = 0.0f;
        m_velocityY = 0.0f;
        m_timer = 0.0f;
        m_frame = 0.0f;
        m_hitFlash = 0.0f;
        m_dead = false;
        m_appeared = false;
        m_terrainAdvanceRequested = false;
        m_shape = RandomPattern(Rng(), false);
        m_fireballs.clear();
        m_orbs.clear();
        m_hitBoxes.clear();
        m_rushFires.clear();
        m_rushIndex = 0;
        m_rushLeftToRight = true;
        m_rushFireDirection = 0;
        m_rushFireRemaining = 0;
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

        if (m_pattern != 14) {
            if (m_hp <= 0) {
                BeginPattern(14);
            } else if (m_hp <= 300 && m_phase < 2 && m_pattern != 11) {
                BeginPattern(11);
            } else if (m_hp <= 550 && m_phase < 1 && m_pattern != 6 && m_pattern != 11) {
                BeginPattern(6);
            }
        }

        UpdatePattern(deltaTime, player);
        UpdateFireballs(deltaTime, player);
        UpdateOrbs(deltaTime, player);
        UpdateHitBoxes(deltaTime, player);
        UpdateRushFire(deltaTime, player);
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
        DrawAttacks(renderer);

        if (!m_appeared) {
            const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, 8);
            renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Appear", frame), m_x, m_y, 64.0f * 3.0f, 64.0f * 3.0f);
            return;
        }

        const Sprite sprite = CurrentSprite();
        if (!sprite.path.empty()) {
            renderer.DrawImage(sprite.path, m_x, m_y, sprite.width, sprite.height, m_hitFlash > 0.0f ? 0.45f : 1.0f);
        }

        if (m_pattern != 14) {
            DrawPattern(renderer, m_shape, m_x, m_y + 95.0f, 0.4f);
            DrawBossHp(renderer, m_hp, m_maxHp);
        }
    }

    bool IsDead() const { return m_dead; }
    bool HasAppeared() const { return m_appeared; }
    bool InDeath() const { return m_pattern == 14; }

    bool ConsumeTerrainAdvance() {
        const bool requested = m_terrainAdvanceRequested;
        m_terrainAdvanceRequested = false;
        return requested;
    }

private:
    struct Sprite {
        std::wstring path;
        float width = 192.0f;
        float height = 192.0f;
    };

    void BeginPattern(int pattern) {
        m_pattern = pattern;
        m_subState = 0;
        m_timer = 0.0f;
        m_frame = 0.0f;
        m_targetX = 0.0f;
        m_velocityX = 0.0f;
        m_velocityY = 0.0f;

        if (pattern == 3) {
            m_orbs.clear();
            m_spawnedOrbs = 0;
        } else if (pattern == 9 || pattern == 10) {
            m_hitBoxes.clear();
        } else if (pattern == 13) {
            m_rushLanes = RandomInt(0, 1) == 0 ? std::array<int, 3> { 0, 1, 0 } : std::array<int, 3> { 1, 0, 1 };
            m_rushIndex = 0;
            m_rushLeftToRight = RandomInt(0, 1) == 0;
            m_rushFireDirection = RandomInt(0, 1);
            m_rushFireRemaining = 15;
            m_rushFires.clear();
            m_rushFires.reserve(16);
        } else if (pattern == 14) {
            m_shape = PatternId::Unknown;
            m_fireballs.clear();
            m_orbs.clear();
            m_hitBoxes.clear();
            m_rushFires.clear();
        }
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

    void UpdatePattern(float deltaTime, Player& player) {
        switch (m_pattern) {
        case 1:
            if (m_timer >= 1.0f) {
                BeginRandomCombatPattern();
            }
            break;
        case 2:
            UpdateJump(deltaTime, player, 1, true);
            break;
        case 3:
            UpdateSpreadFire(deltaTime, player);
            break;
        case 4:
            UpdateHumanRushScratch(deltaTime, player);
            break;
        case 5:
            UpdateHumanVerticalScratch(deltaTime, player);
            break;
        case 6:
            if (m_timer >= 3.0f) {
                m_phase = 1;
                m_terrainAdvanceRequested = true;
                BeginPattern(7);
            }
            break;
        case 7:
            if (m_timer >= 1.0f) {
                BeginRandomCombatPattern();
            }
            break;
        case 8:
            UpdateJump(deltaTime, player, 7, true);
            break;
        case 9:
            UpdateMultiScratch(deltaTime, player);
            break;
        case 10:
            UpdateBite(deltaTime, player);
            break;
        case 11:
            if (m_timer >= 3.0f) {
                m_phase = 2;
                m_terrainAdvanceRequested = true;
                BeginPattern(12);
            }
            break;
        case 12:
            if (m_timer >= 1.0f) {
                BeginPattern(13);
            }
            break;
        case 13:
            UpdateBurningRush(deltaTime, player);
            break;
        case 14:
            if (m_timer >= 6.0f) {
                m_dead = true;
            }
            break;
        default:
            break;
        }
    }

    void UpdateJump(float deltaTime, Player& player, int returnPattern, bool launchOnLand) {
        if (m_subState == 0) {
            if (m_frame >= 2.9f) {
                m_targetX = player.X();
                m_velocityX = (m_targetX - m_x) / 1.2f;
                m_velocityY = 2000.0f * 0.6f;
                m_subState = 1;
                m_frame = 0.0f;
            }
            return;
        }

        if (m_subState == 1 || m_subState == 2) {
            m_x += m_velocityX * deltaTime;
            m_velocityY -= 2000.0f * deltaTime;
            m_y += m_velocityY * deltaTime;
            if (m_velocityY < 0.0f) {
                m_subState = 2;
            }

            if (m_y <= BossGroundY) {
                m_y = BossGroundY;
                m_velocityX = 0.0f;
                m_velocityY = 0.0f;
                m_hitBoxes.push_back({ CenterBox(m_x, 280.0f, 240.0f, 120.0f), 0.0f, 0.0f, 0.35f, 0.0f, 3 });
                if (launchOnLand) {
                    LaunchArcFireballs(m_x);
                }
                m_subState = 3;
                m_timer = 0.0f;
                m_frame = 0.0f;
            }
            return;
        }

        if (m_timer >= 0.55f) {
            BeginPattern(returnPattern);
        }
    }

    void UpdateSpreadFire(float, Player& player) {
        static constexpr std::array<std::array<float, 2>, 5> offsets {{
            {{ -50.0f, 100.0f }}, {{ -100.0f, 0.0f }}, {{ -50.0f, -100.0f }}, {{ 50.0f, -100.0f }}, {{ 100.0f, 0.0f }}
        }};

        if (m_subState == 0 && m_timer >= 0.22f && m_spawnedOrbs < static_cast<int>(offsets.size())) {
            Projectile orb;
            orb.x = m_x + offsets[static_cast<size_t>(m_spawnedOrbs)][0];
            orb.y = m_y + offsets[static_cast<size_t>(m_spawnedOrbs)][1];
            orb.radius = 22.0f;
            m_orbs.push_back(orb);
            ++m_spawnedOrbs;
            m_timer = 0.0f;
        }

        if (m_subState == 0 && m_spawnedOrbs >= static_cast<int>(offsets.size()) && m_timer >= 0.45f) {
            for (Projectile& orb : m_orbs) {
                const float d = std::max(1.0f, Distance(orb.x, orb.y, player.X(), player.Y()));
                orb.vx = (player.X() - orb.x) / d * 600.0f;
                orb.vy = (player.Y() - orb.y) / d * 600.0f;
                orb.life = 1.0f;
            }
            m_subState = 1;
        }

        if (m_subState == 1 && m_orbs.empty()) {
            BeginPattern(1);
        }
    }

    void UpdateHumanRushScratch(float deltaTime, Player& player) {
        if (m_subState == 0) {
            if (m_targetX == 0.0f) {
                m_targetX = player.X();
            }
            const float dx = m_targetX - m_x;
            const float step = std::clamp(dx, -900.0f * deltaTime, 900.0f * deltaTime);
            m_x += step;
            if (m_timer >= 0.5f) {
                m_hitBoxes.push_back({ CenterBox(m_x, m_y, 256.0f, 256.0f), 0.0f, 0.0f, 0.35f, 0.0f, 1 });
                m_subState = 1;
                m_timer = 0.0f;
            }
            return;
        }

        if (m_timer >= 0.75f) {
            BeginPattern(1);
        }
    }

    void UpdateHumanVerticalScratch(float, Player& player) {
        if (m_subState == 0) {
            if (m_targetX == 0.0f) {
                m_targetX = player.X();
            }
            if (m_timer >= 1.0f) {
                m_hitBoxes.push_back({ CenterBox(m_targetX, 300.0f, 108.0f, 440.0f), 0.0f, 0.0f, 0.5f, 0.0f, 2 });
                m_subState = 1;
                m_timer = 0.0f;
            }
            return;
        }

        if (m_timer >= 0.65f) {
            BeginPattern(1);
        }
    }

    void UpdateMultiScratch(float, Player& player) {
        if (m_subState < 4 && m_timer >= 0.35f) {
            m_hitBoxes.push_back({ CenterBox(player.X(), 300.0f, 108.0f, 440.0f), 0.0f, 0.5f, 0.2f, 0.0f, 2 });
            ++m_subState;
            m_timer = 0.0f;
            return;
        }

        if (m_subState >= 4 && m_hitBoxes.empty()) {
            BeginPattern(7);
        }
    }

    void UpdateBite(float, Player&) {
        if (m_subState == 0 && m_timer >= 0.5f) {
            const float direction = m_x < CanvasWidth * 0.5f ? 1.0f : -1.0f;
            for (int i = 0; i < 3; ++i) {
                const float x = m_x + direction * static_cast<float>(i + 1) * 320.0f;
                TimedHitBox box;
                box.box = CenterBox(x, m_y, 160.0f, 160.0f);
                box.prepare = 0.5f + static_cast<float>(i) * 0.2f;
                box.active = 0.35f;
                box.kind = 4;
                m_hitBoxes.push_back(box);
            }
            m_subState = 1;
        }

        if (m_subState == 1 && m_hitBoxes.empty() && m_timer >= 2.1f) {
            BeginPattern(7);
        }
    }

    void UpdateBurningRush(float deltaTime, Player& player) {
        if (m_subState == 0) {
            if (m_timer >= 0.35f) {
                StartRushLeg(player);
                m_subState = 1;
                m_timer = 0.0f;
            }
            return;
        }

        if (m_subState >= 1 && m_subState <= 3) {
            if (m_timer < 1.0f) {
                return;
            }

            m_x += (m_rushLeftToRight ? 1.0f : -1.0f) * 2000.0f * deltaTime;
            DamagePlayerOnTouch(player, CenterBox(m_x, m_y, 192.0f, 64.0f));

            const bool ended = m_rushLeftToRight ? m_x >= CanvasWidth + 500.0f : m_x <= -500.0f;
            if (!ended) {
                return;
            }

            ++m_rushIndex;
            if (m_rushIndex >= m_rushLanes.size()) {
                m_x = 1000.0f;
                m_y = BossGroundY;
                m_subState = 4;
                m_timer = 0.0f;
                m_frame = 0.0f;
            } else {
                StartRushLeg(player);
                ++m_subState;
                m_timer = 0.0f;
            }
            return;
        }

        if (m_subState == 4 && m_timer >= 0.85f) {
            BeginPattern(7);
        }
    }

    void StartRushLeg(const Player& player) {
        m_rushLeftToRight = player.X() > CanvasWidth * 0.5f ? false : true;
        if (m_rushIndex % 2 == 1) {
            m_rushLeftToRight = !m_rushLeftToRight;
        }
        m_x = m_rushLeftToRight ? -500.0f : CanvasWidth + 500.0f;
        m_y = m_rushLanes[m_rushIndex] == 0 ? BossGroundY : 380.0f;
    }

    void LaunchArcFireballs(float startX) {
        static constexpr std::array<float, 5> angles { -60.0f, -30.0f, 0.0f, 30.0f, 60.0f };
        for (float angleDeg : angles) {
            const float angle = angleDeg * Pi / 180.0f;
            Projectile p;
            p.x = startX;
            p.y = BossGroundY;
            p.vx = std::sin(angle) * 700.0f;
            p.vy = 780.0f;
            p.radius = 22.0f;
            m_fireballs.push_back(p);
        }
    }

    void UpdateFireballs(float deltaTime, Player& player) {
        for (Projectile& p : m_fireballs) {
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.vy -= 1600.0f * deltaTime;
            p.frame = std::fmod(p.frame + 4.0f * deltaTime, 4.0f);
            if (CircleIntersects(player.Bounds(), p.x, p.y, p.radius)) {
                player.TakeDamage(1);
                p.life = -999.0f;
            }
        }

        m_fireballs.erase(std::remove_if(m_fireballs.begin(), m_fireballs.end(), [](const Projectile& p) {
            return p.life < -100.0f || p.x < -120.0f || p.x > CanvasWidth + 120.0f || p.y < -120.0f || p.y > CanvasHeight + 220.0f;
        }), m_fireballs.end());
    }

    void UpdateOrbs(float deltaTime, Player& player) {
        for (Projectile& orb : m_orbs) {
            orb.frame = std::fmod(orb.frame + 4.0f * deltaTime, 4.0f);
            if (orb.life > 0.5f) {
                orb.x += orb.vx * deltaTime;
                orb.y += orb.vy * deltaTime;
            }
            if (CircleIntersects(player.Bounds(), orb.x, orb.y, orb.radius)) {
                player.TakeDamage(1);
                orb.life = -999.0f;
            }
        }

        m_orbs.erase(std::remove_if(m_orbs.begin(), m_orbs.end(), [](const Projectile& orb) {
            return orb.life < -100.0f || orb.x < -120.0f || orb.x > CanvasWidth + 120.0f || orb.y < -120.0f || orb.y > CanvasHeight + 120.0f;
        }), m_orbs.end());
    }

    void UpdateHitBoxes(float deltaTime, Player& player) {
        for (TimedHitBox& hit : m_hitBoxes) {
            hit.elapsed += deltaTime;
            hit.frame += 8.0f * deltaTime;
            if (hit.elapsed >= hit.prepare && hit.elapsed <= hit.prepare + hit.active) {
                DamagePlayerOnTouch(player, hit.box);
            }
        }

        m_hitBoxes.erase(std::remove_if(m_hitBoxes.begin(), m_hitBoxes.end(), [](const TimedHitBox& hit) {
            return hit.elapsed > hit.prepare + hit.active + 0.15f;
        }), m_hitBoxes.end());
    }

    void UpdateRushFire(float deltaTime, Player& player) {
        if (m_pattern != 13) {
            return;
        }

        if (m_rushFireRemaining == 15) {
            m_rushFires.push_back({ m_rushFireDirection == 0 ? 100.0f : CanvasWidth + 620.0f, 50.0f, 0.0f, m_rushFireDirection, false, false });
            --m_rushFireRemaining;
        }

        for (size_t i = 0; i < m_rushFires.size(); ++i) {
            RushFire& fire = m_rushFires[i];
            fire.frame += 8.0f * deltaTime;
            if (!fire.active && fire.frame >= 4.0f) {
                fire.active = true;
                fire.frame = 0.0f;
                if (m_rushFireRemaining > 0) {
                    if (m_rushFireRemaining > 1) {
                        const float offset = (m_rushFireRemaining % 2 == 1) ? -30.0f : 30.0f;
                        m_rushFires.push_back({ fire.x + offset, 50.0f + static_cast<float>(15 - m_rushFireRemaining) * 40.0f,
                            0.0f, m_rushFireDirection, false, false });
                    }
                    --m_rushFireRemaining;
                    if (m_rushFireRemaining == 0) {
                        for (RushFire& each : m_rushFires) {
                            each.released = true;
                        }
                    }
                }
            }

            if (fire.active) {
                DamagePlayerOnTouch(player, CenterBox(fire.x, fire.y, 64.0f, 64.0f));
                if (fire.released) {
                    fire.x += (fire.direction == 0 ? 1.0f : -1.0f) * 200.0f * deltaTime;
                }
            }
        }

        m_rushFires.erase(std::remove_if(m_rushFires.begin(), m_rushFires.end(), [](const RushFire& fire) {
            return fire.x < -80.0f || fire.x > CanvasWidth + 720.0f;
        }), m_rushFires.end());
    }

    void DrawAttacks(Renderer& renderer) const {
        for (const Projectile& p : m_fireballs) {
            const int frame = std::clamp(static_cast<int>(p.frame) + 1, 1, 4);
            renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Fire", frame), p.x, p.y, 64.0f, 64.0f);
        }

        for (const Projectile& orb : m_orbs) {
            const int frame = std::clamp(static_cast<int>(orb.frame) + 1, 1, 4);
            renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Fire b", frame), orb.x, orb.y, 64.0f, 64.0f);
        }

        for (const TimedHitBox& hit : m_hitBoxes) {
            const float cx = (hit.box.left + hit.box.right) * 0.5f;
            const float cy = (hit.box.bottom + hit.box.top) * 0.5f;
            const float width = hit.box.right - hit.box.left;
            const float height = hit.box.top - hit.box.bottom;
            if (hit.elapsed < hit.prepare) {
                const std::wstring warning = hit.kind == 4 ? L"Assets\\3stage\\Fox Bite Warning.png" : L"Assets\\3stage\\Fox Scratch Warning.png";
                renderer.DrawImage(warning, cx, cy, width, height, 0.65f);
            } else if (hit.kind == 4) {
                const int frame = std::clamp(static_cast<int>((hit.elapsed - hit.prepare) * 12.0f) + 1, 1, 5);
                renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Bite", frame), cx, cy, width, height);
            } else {
                const int frame = std::clamp(static_cast<int>((hit.elapsed - hit.prepare) * 12.0f) + 1, 1, 3);
                renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Scratch", frame), cx, cy, width, height);
            }
        }

        if (m_pattern == 13 && m_subState >= 1 && m_subState <= 3 && m_timer < 1.0f) {
            renderer.DrawImage(L"Assets\\3stage\\Rush Warning.png", CanvasWidth * 0.5f, m_y, CanvasWidth, 64.0f, 0.7f);
        }

        for (const RushFire& fire : m_rushFires) {
            const int frame = std::clamp(static_cast<int>(std::fmod(fire.frame, 4.0f)) + 1, 1, 4);
            renderer.DrawImage(NumberedAsset(L"Assets\\3stage\\Fox Fire b", frame), fire.x, fire.y, 64.0f, 64.0f, fire.active ? 1.0f : 0.55f);
        }
    }

    Sprite CurrentSprite() const {
        if (m_pattern == 14) {
            const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, 18);
            return { NumberedAsset(L"Assets\\3stage\\Die", frame), 96.0f * 3.0f, 96.0f * 3.0f };
        }

        if (m_pattern == 6) {
            const int frame = std::clamp(static_cast<int>(m_timer / 3.0f * 18.0f) + 1, 1, 18);
            return { NumberedAsset(L"Assets\\3stage\\Change Phase 1-", frame), 128.0f * 2.0f, 64.0f * 2.0f };
        }

        if (m_pattern == 11) {
            const int frame = std::clamp(static_cast<int>(m_timer / 3.0f * 24.0f), 0, 23);
            const int group = frame / 8;
            const int index = frame % 8 + 1;
            const wchar_t groupName = group == 0 ? L'a' : (group == 1 ? L'b' : L'c');
            return { L"Assets\\3stage\\Change Phase 2-" + std::wstring(1, groupName) + std::to_wstring(index) + L".png",
                128.0f * 2.0f, 128.0f * 2.0f };
        }

        if (m_pattern == 13) {
            if (m_subState == 0) {
                const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, 3);
                return { NumberedAsset(L"Assets\\3stage\\Rush Prepare", frame), 128.0f * 2.0f, 128.0f * 2.0f };
            }
            if (m_subState == 4) {
                const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, 8);
                return { NumberedAsset(L"Assets\\3stage\\Rush Over", frame), 128.0f * 2.0f, 128.0f * 2.0f };
            }
            return { L"Assets\\3stage\\Rush Particle.png", 192.0f, 64.0f };
        }

        if (m_pattern == 9) {
            const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, 3);
            return { NumberedAsset(L"Assets\\3stage\\Scratch Prepare", frame), 128.0f * 2.0f, 128.0f * 2.0f };
        }

        if (m_pattern == 10) {
            if (m_subState == 0) {
                return { L"Assets\\3stage\\Bite Prepare 1.png", 160.0f * 2.0f, 160.0f * 2.0f };
            }
            return { L"Assets\\3stage\\Bite Cast 1.png", 160.0f * 2.0f, 160.0f * 2.0f };
        }

        if (m_pattern == 4) {
            const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, 3);
            return { NumberedAsset(L"Assets\\3stage\\Fox Human Scratch Rush Prepare", frame), 128.0f * 2.0f, 128.0f * 2.0f };
        }

        if (m_pattern == 5) {
            const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, 3);
            return { NumberedAsset(L"Assets\\3stage\\Fox Human Scratch Prepare", frame), 128.0f * 2.0f, 128.0f * 2.0f };
        }

        if (m_pattern == 2 || m_pattern == 8) {
            const std::wstring prefix = m_phase == 0 ? L"Assets\\3stage\\Fox Human Jump Prepare" : L"Assets\\3stage\\Jump Prepare";
            const int maxFrame = m_phase == 0 ? 3 : 2;
            const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, maxFrame);
            return { NumberedAsset(prefix, frame), 128.0f * 2.0f, 128.0f * 2.0f };
        }

        if (m_pattern == 3) {
            const int frame = std::clamp(static_cast<int>(m_frame) + 1, 1, 4);
            return { NumberedAsset(L"Assets\\3stage\\Fox Human Spread Fire Prepare", frame), 128.0f * 2.0f, 128.0f * 2.0f };
        }

        if (m_phase >= 2) {
            const int frame = static_cast<int>(std::fmod(m_frame, 8.0f)) + 1;
            return { NumberedAsset(L"Assets\\3stage\\Fox Burning Idle", frame), 96.0f * 3.0f, 96.0f * 3.0f };
        }

        if (m_phase >= 1) {
            const int frame = static_cast<int>(std::fmod(m_frame, 8.0f)) + 1;
            return { NumberedAsset(L"Assets\\3stage\\Idle", frame), 96.0f * 3.0f, 96.0f * 3.0f };
        }

        const int frame = static_cast<int>(std::fmod(m_frame, 2.0f)) + 1;
        return { NumberedAsset(L"Assets\\3stage\\Fox Human Idle", frame), 64.0f * 3.0f, 64.0f * 3.0f };
    }

private:
    int m_maxHp = 800;
    int m_hp = 800;
    int m_pattern = 0;
    int m_subState = 0;
    int m_phase = 0;

    float m_x = 1000.0f;
    float m_y = BossGroundY;
    float m_targetX = 0.0f;
    float m_velocityX = 0.0f;
    float m_velocityY = 0.0f;
    float m_timer = 0.0f;
    float m_frame = 0.0f;
    float m_hitFlash = 0.0f;

    bool m_dead = false;
    bool m_appeared = false;
    bool m_terrainAdvanceRequested = false;
    PatternId m_shape = PatternId::Unknown;

    int m_spawnedOrbs = 0;
    std::vector<Projectile> m_fireballs;
    std::vector<Projectile> m_orbs;
    std::vector<TimedHitBox> m_hitBoxes;

    std::array<int, 3> m_rushLanes { 0, 1, 0 };
    size_t m_rushIndex = 0;
    bool m_rushLeftToRight = true;
    int m_rushFireDirection = 0;
    int m_rushFireRemaining = 0;
    std::vector<RushFire> m_rushFires;
};

class Stage3Controller final : public StageController {
public:
    void Reset() override {
        m_cleared = false;
        m_terrain.Reset();
        m_boss.Reset();
    }

    void Update(float deltaTime, Player& player) override {
        m_boss.Update(deltaTime, player);
        if (m_boss.ConsumeTerrainAdvance()) {
            m_terrain.AdvancePattern();
        }
        if (m_boss.HasAppeared() && !m_boss.InDeath()) {
            m_terrain.Update(deltaTime, player);
        }
        if (m_boss.IsDead()) {
            m_cleared = true;
        }
    }

    void ApplyGesture(PatternId id, Player& player) override {
        if (m_boss.ApplyGesture(id, player.AttackPower())) {
            player.QueueSmash();
        }
    }

    void Draw(Renderer& renderer, const Player&) const override {
        m_terrain.Draw(renderer, m_boss.HasAppeared() && !m_boss.InDeath());
        m_boss.Draw(renderer);
    }

    bool IsCleared() const override { return m_cleared; }

private:
    Stage3Terrain m_terrain;
    SihoBoss m_boss;
    bool m_cleared = false;
};

} // namespace

std::unique_ptr<StageController> CreateStage3Controller() {
    auto controller = std::make_unique<Stage3Controller>();
    controller->Reset();
    return controller;
}
