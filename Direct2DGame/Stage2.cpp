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

class KittyBoss;
class KittyIdleState;
class KittyPattern0State;
class KittyPattern1State;
class KittyPattern2State;
class KittyPattern3State;
class KittyDieState;

enum class LaserMode {
    Active,
    Fade
};

struct KittyBullet {
    Projectile body {};
    float effectTimer = 0.0f;
};

struct KittyEffect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 20.0f;
    float height = 20.0f;
    float frame = 0.0f;
};

struct KittyLaser {
    float y = 0.0f;
    float timer = 0.0f;
    float frame = 0.0f;
    LaserMode mode = LaserMode::Active;
};

struct LittleKitty {
    float originX = 0.0f;
    float y = 0.0f;
    float age = 0.0f;
};

class KittyState {
public:
    virtual ~KittyState() = default;
    virtual void Enter(KittyBoss& boss) = 0;
    virtual void Update(KittyBoss& boss, float deltaTime, Player& player) = 0;
};

class KittyBoss {
public:
    void Reset();
    void Start();
    void Update(float deltaTime, Player& player);
    bool ApplyGesture(PatternId id, int damage);
    void Draw(Renderer& renderer) const;

    bool IsDead() const { return m_dead; }
    bool Started() const { return m_started; }

    void ChangeState(std::unique_ptr<KittyState> next);

private:
    friend class KittyIdleState;
    friend class KittyPattern0State;
    friend class KittyPattern1State;
    friend class KittyPattern2State;
    friend class KittyPattern3State;
    friend class KittyDieState;

    void ClearAttacks();
    void SpawnHoming(const Player& player);
    void SpawnLittleKitty();
    void SpawnFanBullet();
    void UpdateEffects(float deltaTime);
    void UpdateHoming(float deltaTime, Player& player);
    void UpdateLasers(float deltaTime, Player& player);
    void UpdateLittleKitties(float deltaTime, Player& player);
    void UpdateFanBullets(float deltaTime, Player& player);
    void DrawAttacks(Renderer& renderer) const;

private:
    std::unique_ptr<KittyState> m_state;
    bool m_started = false;
    bool m_dead = false;
    bool m_inDeath = false;

    int m_maxHp = 360;
    int m_hp = 360;
    float m_x = CanvasWidth - 300.0f;
    float m_y = CanvasHeight * 0.5f;
    float m_dir = 1.0f;
    float m_idleFrame = 0.0f;
    float m_hitFlash = 0.0f;

    int m_homingRemaining = 0;
    int m_laserRemaining = 0;
    int m_littleKittySpawned = 0;
    float m_spawnTimer = 0.0f;
    float m_patternTimer = 0.0f;
    float m_laserChargeTimer = 0.0f;
    float m_laserWarningY = 0.0f;
    bool m_laserWarningVisible = false;

    PatternId m_shape = PatternId::Unknown;
    std::vector<KittyBullet> m_homingBullets;
    std::vector<KittyEffect> m_effects;
    std::vector<KittyLaser> m_lasers;
    std::vector<LittleKitty> m_littleKitties;
    std::vector<Projectile> m_fanBullets;
};

class KittyIdleState final : public KittyState {
public:
    void Enter(KittyBoss& boss) override { boss.m_patternTimer = 0.0f; }
    void Update(KittyBoss& boss, float, Player&) override;
};

class KittyPattern0State final : public KittyState {
public:
    void Enter(KittyBoss& boss) override;
    void Update(KittyBoss& boss, float deltaTime, Player& player) override;
};

class KittyPattern1State final : public KittyState {
public:
    void Enter(KittyBoss& boss) override;
    void Update(KittyBoss& boss, float deltaTime, Player& player) override;
};

class KittyPattern2State final : public KittyState {
public:
    void Enter(KittyBoss& boss) override;
    void Update(KittyBoss& boss, float deltaTime, Player& player) override;
};

class KittyPattern3State final : public KittyState {
public:
    void Enter(KittyBoss& boss) override;
    void Update(KittyBoss& boss, float deltaTime, Player& player) override;
};

class KittyDieState final : public KittyState {
public:
    void Enter(KittyBoss& boss) override;
    void Update(KittyBoss& boss, float deltaTime, Player&) override;
};

void KittyBoss::Reset() {
    m_state.reset();
    m_started = false;
    m_dead = false;
    m_inDeath = false;
    m_hp = m_maxHp;
    m_x = CanvasWidth - 300.0f;
    m_y = CanvasHeight * 0.5f;
    m_dir = 1.0f;
    m_idleFrame = 0.0f;
    m_hitFlash = 0.0f;
    m_shape = RandomPattern(Rng(), false);
    ClearAttacks();
}

void KittyBoss::Start() {
    if (m_started) {
        return;
    }
    m_started = true;
    ChangeState(std::make_unique<KittyPattern0State>());
}

void KittyBoss::Update(float deltaTime, Player& player) {
    if (!m_started || m_dead) {
        return;
    }

    m_idleFrame = std::fmod(m_idleFrame + 8.0f * deltaTime, 2.0f);
    m_hitFlash = std::max(0.0f, m_hitFlash - deltaTime);

    if (!m_inDeath) {
        m_y += 300.0f * deltaTime * m_dir;
        if (m_y >= CanvasHeight - 170.0f) {
            m_y = CanvasHeight - 170.0f;
            m_dir = -1.0f;
        } else if (m_y <= 170.0f) {
            m_y = 170.0f;
            m_dir = 1.0f;
        }
    }

    if (m_hp <= 0 && !m_inDeath) {
        ChangeState(std::make_unique<KittyDieState>());
    }

    if (m_state) {
        m_state->Update(*this, deltaTime, player);
    }

    UpdateEffects(deltaTime);
    UpdateHoming(deltaTime, player);
    UpdateLasers(deltaTime, player);
    UpdateLittleKitties(deltaTime, player);
    UpdateFanBullets(deltaTime, player);
}

bool KittyBoss::ApplyGesture(PatternId id, int damage) {
    if (!m_started || m_inDeath || id != m_shape || m_hp <= 0) {
        return false;
    }

    m_hp = std::max(0, m_hp - damage);
    m_hitFlash = 0.18f;
    m_shape = RandomPattern(Rng(), false);
    return true;
}

void KittyBoss::Draw(Renderer& renderer) const {
    if (!m_started) {
        DrawPattern(renderer, PatternId::Heart, CanvasWidth * 0.5f, CanvasHeight - 110.0f, 0.6f);
        renderer.DrawTextPico(L"하트를 그리면 전투가 시작돼", 430.0f, 625.0f, 420.0f, 36.0f, 26.0f,
            D2D1::ColorF(D2D1::ColorF::White), DWRITE_TEXT_ALIGNMENT_CENTER);
        return;
    }

    DrawAttacks(renderer);

    if (!m_dead) {
        const std::wstring texture = (static_cast<int>(m_idleFrame) == 0) ? L"Assets\\2stage\\boss1.png" : L"Assets\\2stage\\boss2.png";
        renderer.DrawImage(texture, m_x, m_y, 362.0f * 1.2f, 288.0f * 1.2f, m_hitFlash > 0.0f ? 0.45f : 1.0f);
    }

    if (!m_inDeath) {
        DrawPattern(renderer, m_shape, m_x, m_y + 95.0f, 0.6f);
        DrawBossHp(renderer, m_hp, m_maxHp);
    }
}

void KittyBoss::ChangeState(std::unique_ptr<KittyState> next) {
    m_state = std::move(next);
    if (m_state) {
        m_state->Enter(*this);
    }
}

void KittyBoss::ClearAttacks() {
    m_homingBullets.clear();
    m_effects.clear();
    m_lasers.clear();
    m_littleKitties.clear();
    m_fanBullets.clear();
    m_laserWarningVisible = false;
    m_homingRemaining = 0;
    m_laserRemaining = 0;
    m_littleKittySpawned = 0;
    m_spawnTimer = 0.0f;
    m_patternTimer = 0.0f;
    m_laserChargeTimer = 0.0f;
}

void KittyBoss::SpawnHoming(const Player& player) {
    KittyBullet bullet;
    bullet.body.x = m_x;
    bullet.body.y = m_y;
    bullet.body.radius = 16.0f;

    const float d = std::max(1.0f, Distance(bullet.body.x, bullet.body.y, player.X(), player.Y()));
    bullet.body.vx = (player.X() - bullet.body.x) / d * 1200.0f;
    bullet.body.vy = (player.Y() - bullet.body.y) / d * 1200.0f;
    m_homingBullets.push_back(bullet);
}

void KittyBoss::SpawnLittleKitty() {
    LittleKitty kitty;
    kitty.originX = RandomRange(160.0f, CanvasWidth - 160.0f);
    kitty.y = CanvasHeight + 50.0f;
    m_littleKitties.push_back(kitty);
}

void KittyBoss::SpawnFanBullet() {
    const float angle = RandomRange(135.0f, 225.0f) * 3.14159265f / 180.0f;
    Projectile bullet;
    bullet.x = m_x;
    bullet.y = m_y;
    bullet.vx = std::cos(angle) * 200.0f;
    bullet.vy = std::sin(angle) * 200.0f;
    bullet.radius = 15.0f;
    m_fanBullets.push_back(bullet);
}

void KittyBoss::UpdateEffects(float deltaTime) {
    for (KittyEffect& effect : m_effects) {
        effect.frame = std::fmod(effect.frame + 40.0f * deltaTime, 28.0f);
        effect.width -= 8.0f * deltaTime;
        effect.height -= 8.0f * deltaTime;
    }

    m_effects.erase(std::remove_if(m_effects.begin(), m_effects.end(), [](const KittyEffect& effect) {
        return effect.width <= 0.0f || effect.height <= 0.0f;
    }), m_effects.end());
}

void KittyBoss::UpdateHoming(float deltaTime, Player& player) {
    for (KittyBullet& bullet : m_homingBullets) {
        bullet.body.x += bullet.body.vx * deltaTime;
        bullet.body.y += bullet.body.vy * deltaTime;
        bullet.body.frame = std::fmod(bullet.body.frame + 40.0f * deltaTime, 28.0f);
        bullet.effectTimer += deltaTime;

        if (bullet.effectTimer >= 0.2f) {
            m_effects.push_back({ bullet.body.x, bullet.body.y, 20.0f, 20.0f, bullet.body.frame });
            bullet.effectTimer = 0.0f;
        }

        if (CircleIntersects(player.Bounds(), bullet.body.x, bullet.body.y, bullet.body.radius)) {
            player.TakeDamage(1);
            bullet.body.life = -999.0f;
        }
    }

    m_homingBullets.erase(std::remove_if(m_homingBullets.begin(), m_homingBullets.end(), [](const KittyBullet& bullet) {
        const Projectile& p = bullet.body;
        return p.life < -100.0f || p.x < -80.0f || p.x > CanvasWidth + 80.0f || p.y < -80.0f || p.y > CanvasHeight + 80.0f;
    }), m_homingBullets.end());
}

void KittyBoss::UpdateLasers(float deltaTime, Player& player) {
    for (KittyLaser& laser : m_lasers) {
        laser.timer += deltaTime;
        laser.frame += 40.0f * deltaTime;

        if (laser.mode == LaserMode::Active && laser.timer < 1.0f && std::abs(player.Y() - laser.y) < 28.0f) {
            player.TakeDamage(1);
        }
        if (laser.timer >= 1.0f) {
            laser.mode = LaserMode::Fade;
        }
    }

    m_lasers.erase(std::remove_if(m_lasers.begin(), m_lasers.end(), [](const KittyLaser& laser) {
        return laser.timer >= 1.55f;
    }), m_lasers.end());
}

void KittyBoss::UpdateLittleKitties(float deltaTime, Player& player) {
    for (LittleKitty& kitty : m_littleKitties) {
        kitty.age += deltaTime;
        kitty.y -= 150.0f * deltaTime;
        const float x = kitty.originX + 50.0f * std::sin(kitty.age * 3.0f);
        if (Intersects(player.Bounds(), CenterBox(x, kitty.y, 50.0f, 50.0f))) {
            player.TakeDamage(1);
            kitty.y = -999.0f;
        }
    }

    m_littleKitties.erase(std::remove_if(m_littleKitties.begin(), m_littleKitties.end(), [](const LittleKitty& kitty) {
        return kitty.y < -80.0f;
    }), m_littleKitties.end());
}

void KittyBoss::UpdateFanBullets(float deltaTime, Player& player) {
    for (Projectile& bullet : m_fanBullets) {
        bullet.x += bullet.vx * deltaTime;
        bullet.y += bullet.vy * deltaTime;
        bullet.frame = std::fmod(bullet.frame + 40.0f * deltaTime, 28.0f);

        if (CircleIntersects(player.Bounds(), bullet.x, bullet.y, bullet.radius)) {
            player.TakeDamage(1);
            bullet.life = -999.0f;
        }
    }

    m_fanBullets.erase(std::remove_if(m_fanBullets.begin(), m_fanBullets.end(), [](const Projectile& bullet) {
        return bullet.life < -100.0f || bullet.x < -80.0f || bullet.x > CanvasWidth + 80.0f || bullet.y < -80.0f || bullet.y > CanvasHeight + 80.0f;
    }), m_fanBullets.end());
}

void KittyBoss::DrawAttacks(Renderer& renderer) const {
    if (m_laserWarningVisible) {
        renderer.FillRect(0.0f, m_laserWarningY - 8.0f, CanvasWidth, m_laserWarningY + 8.0f, D2D1::ColorF(1.0f, 0.2f, 0.55f), 0.28f);
    }

    for (const KittyEffect& effect : m_effects) {
        const int frame = static_cast<int>(std::fmod(effect.frame, 28.0f)) + 1;
        renderer.DrawImage(NumberedAsset(L"Assets\\2stage\\attack_", frame), effect.x, effect.y, effect.width, effect.height, 0.55f);
    }

    for (const KittyBullet& bullet : m_homingBullets) {
        const int frame = static_cast<int>(std::fmod(bullet.body.frame, 28.0f)) + 1;
        renderer.DrawImage(NumberedAsset(L"Assets\\2stage\\attack_", frame), bullet.body.x, bullet.body.y, 40.0f, 40.0f);
    }

    for (const Projectile& bullet : m_fanBullets) {
        const int frame = static_cast<int>(std::fmod(bullet.frame, 28.0f)) + 1;
        renderer.DrawImage(NumberedAsset(L"Assets\\2stage\\attack_", frame), bullet.x, bullet.y, 32.0f, 32.0f);
    }

    for (const KittyLaser& laser : m_lasers) {
        const int frame = std::clamp(static_cast<int>(laser.frame), 1, 4);
        const float opacity = laser.mode == LaserMode::Active ? 0.88f : 0.45f;
        renderer.DrawImage(NumberedAsset(L"Assets\\2stage\\lightAttack", frame), CanvasWidth * 0.5f, laser.y, CanvasWidth, 42.0f, opacity);
    }

    for (const LittleKitty& kitty : m_littleKitties) {
        const float x = kitty.originX + 50.0f * std::sin(kitty.age * 3.0f);
        renderer.DrawImage(L"Assets\\2stage\\157.png", x, kitty.y, 70.0f, 70.0f);
    }
}

void KittyIdleState::Update(KittyBoss& boss, float, Player&) {
    boss.ChangeState(std::make_unique<KittyPattern0State>());
}

void KittyPattern0State::Enter(KittyBoss& boss) {
    boss.ClearAttacks();
    boss.m_homingRemaining = 8;
}

void KittyPattern0State::Update(KittyBoss& boss, float, Player& player) {
    if (boss.m_homingBullets.empty() && boss.m_homingRemaining > 0) {
        boss.SpawnHoming(player);
        --boss.m_homingRemaining;
    }

    if (boss.m_homingRemaining == 0 && boss.m_homingBullets.empty()) {
        boss.ChangeState(std::make_unique<KittyPattern1State>());
    }
}

void KittyPattern1State::Enter(KittyBoss& boss) {
    boss.ClearAttacks();
    boss.m_laserRemaining = 3;
    boss.m_laserWarningY = boss.m_y;
    boss.m_laserWarningVisible = true;
}

void KittyPattern1State::Update(KittyBoss& boss, float deltaTime, Player& player) {
    if (boss.m_lasers.empty() && boss.m_laserRemaining > 0) {
        boss.m_laserChargeTimer += deltaTime;
        boss.m_laserWarningY = player.Y();
        boss.m_laserWarningVisible = true;

        if (boss.m_laserChargeTimer >= 2.0f) {
            boss.m_lasers.push_back({ boss.m_laserWarningY, 0.0f, 1.0f, LaserMode::Active });
            boss.m_laserChargeTimer = 0.0f;
            boss.m_laserWarningVisible = false;
            --boss.m_laserRemaining;
        }
    }

    if (boss.m_laserRemaining == 0 && boss.m_lasers.empty()) {
        boss.ChangeState(std::make_unique<KittyPattern2State>());
    }
}

void KittyPattern2State::Enter(KittyBoss& boss) {
    boss.ClearAttacks();
    boss.m_spawnTimer = 0.0f;
    boss.m_littleKittySpawned = 0;
}

void KittyPattern2State::Update(KittyBoss& boss, float deltaTime, Player&) {
    boss.m_spawnTimer += deltaTime;
    if (boss.m_littleKittySpawned < 8 && boss.m_spawnTimer >= 0.5f) {
        boss.SpawnLittleKitty();
        ++boss.m_littleKittySpawned;
        boss.m_spawnTimer = 0.0f;
    }

    if (boss.m_littleKittySpawned == 8 && boss.m_littleKitties.empty()) {
        boss.ChangeState(std::make_unique<KittyPattern3State>());
    }
}

void KittyPattern3State::Enter(KittyBoss& boss) {
    boss.ClearAttacks();
    boss.m_patternTimer = 0.0f;
    boss.m_spawnTimer = 0.0f;
    boss.m_fanBullets.reserve(256);
}

void KittyPattern3State::Update(KittyBoss& boss, float deltaTime, Player&) {
    boss.m_patternTimer += deltaTime;
    boss.m_spawnTimer += deltaTime;

    if (boss.m_patternTimer < 8.0f && boss.m_spawnTimer >= 0.05f) {
        boss.SpawnFanBullet();
        boss.m_spawnTimer = 0.0f;
    }

    if (boss.m_patternTimer >= 8.0f && boss.m_fanBullets.empty()) {
        boss.ChangeState(std::make_unique<KittyIdleState>());
    }
}

void KittyDieState::Enter(KittyBoss& boss) {
    boss.ClearAttacks();
    boss.m_inDeath = true;
    boss.m_shape = PatternId::Unknown;
}

void KittyDieState::Update(KittyBoss& boss, float deltaTime, Player&) {
    boss.m_y -= 260.0f * deltaTime;
    if (boss.m_y < -200.0f) {
        boss.m_dead = true;
    }
}

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

} // namespace

std::unique_ptr<StageController> CreateStage2Controller() {
    auto controller = std::make_unique<Stage2Controller>();
    controller->Reset();
    return controller;
}
