#include "Boss.h"

#include "Constants.h"
#include "Renderer.h"

#include <algorithm>
#include <cmath>

namespace {
bool Intersects(const HitBox& box, const Projectile& projectile) {
    const float closestX = std::clamp(projectile.x, box.left, box.right);
    const float closestY = std::clamp(projectile.y, box.bottom, box.top);
    const float dx = projectile.x - closestX;
    const float dy = projectile.y - closestY;
    return dx * dx + dy * dy <= projectile.radius * projectile.radius;
}
}

void Boss::Reset(int stage) {
    m_stage = stage;

    if (stage == 2) {
        m_maxHp = 320;
        m_x = 1000.0f;
        m_y = 360.0f;
    } else if (stage == 3) {
        m_maxHp = 500;
        m_x = 980.0f;
        m_y = 330.0f;
    } else {
        m_maxHp = 240;
        m_x = 960.0f;
        m_y = 380.0f;
    }

    m_hp = m_maxHp;
    m_frame = 0.0f;
    m_attackTimer = 1.2f;
    m_hitFlashTimer = 0.0f;
    m_projectiles.clear();
}

void Boss::Update(float deltaTime, Player& player) {
    m_frame += deltaTime * 8.0f;

    if (m_hitFlashTimer > 0.0f) {
        m_hitFlashTimer -= deltaTime;
    }

    m_attackTimer -= deltaTime;
    if (m_attackTimer <= 0.0f && !IsDead()) {
        SpawnProjectile(player);
        m_attackTimer = (m_stage == 3) ? 1.2f : 1.6f;
    }

    for (Projectile& projectile : m_projectiles) {
        projectile.x += projectile.vx * deltaTime;
        projectile.y += projectile.vy * deltaTime;
    }

    for (Projectile& projectile : m_projectiles) {
        if (Intersects(player.Bounds(), projectile) && player.TakeDamage(1)) {
            projectile.x = -9999.0f;
        }
    }

    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(), [](const Projectile& projectile) {
            return projectile.x < -100.0f || projectile.x > CanvasWidth + 100.0f ||
                projectile.y < -100.0f || projectile.y > CanvasHeight + 100.0f;
        }),
        m_projectiles.end()
    );
}

void Boss::Draw(Renderer& renderer) const {
    DrawBody(renderer);

    for (const Projectile& projectile : m_projectiles) {
        D2D1_COLOR_F color = D2D1::ColorF(0.9f, 0.18f, 0.12f, 0.95f);
        if (m_stage == 2) {
            color = D2D1::ColorF(0.95f, 0.55f, 0.95f, 0.95f);
        } else if (m_stage == 3) {
            color = D2D1::ColorF(1.0f, 0.45f, 0.05f, 0.95f);
        }
        renderer.FillEllipse(projectile.x, projectile.y, projectile.radius, projectile.radius, color);
    }

    DrawHp(renderer);
}

void Boss::TakeGestureHit(int damage) {
    if (m_hp <= 0) {
        return;
    }
    m_hp = std::max(0, m_hp - damage);
    m_hitFlashTimer = 0.18f;
}

void Boss::SpawnProjectile(const Player& player) {
    Projectile projectile;
    projectile.x = m_x;
    projectile.y = m_y;
    projectile.radius = (m_stage == 3) ? 22.0f : 18.0f;

    const float dx = player.X() - projectile.x;
    const float dy = player.Y() - projectile.y;
    const float distance = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
    const float speed = (m_stage == 3) ? 360.0f : 290.0f;
    projectile.vx = dx / distance * speed;
    projectile.vy = dy / distance * speed;
    m_projectiles.push_back(projectile);
}

void Boss::DrawBody(Renderer& renderer) const {
    const float opacity = (m_hitFlashTimer > 0.0f) ? 0.45f : 1.0f;

    if (m_stage == 1) {
        // 원본 resource.py의 boss_ghost_idle_coordinate를 Direct2D clip_draw로 옮긴 좌표다.
        const float frames[4][4] = {
            {3313, 7, 70, 104}, {3385, 7, 67, 104}, {3457, 5, 66, 106}, {3529, 7, 66, 104}
        };
        const int index = static_cast<int>(m_frame) % 4;
        renderer.DrawImageClip(L"Assets\\1stage\\level1-png-sprite.png", frames[index][0], frames[index][1], frames[index][2], frames[index][3],
            m_x, m_y, frames[index][2] * 1.9f, frames[index][3] * 1.9f, false, opacity);
        return;
    }

    if (m_stage == 2) {
        const std::wstring texture = (static_cast<int>(m_frame) % 2 == 0) ? L"Assets\\2stage\\boss1.png" : L"Assets\\2stage\\boss2.png";
        renderer.DrawImage(texture, m_x, m_y, 362.0f * 1.15f, 288.0f * 1.15f, opacity);
        return;
    }

    if (m_hp > m_maxHp / 2) {
        const std::wstring texture = (static_cast<int>(m_frame) % 2 == 0) ? L"Assets\\3stage\\Fox Human Idle1.png" : L"Assets\\3stage\\Fox Human Idle2.png";
        renderer.DrawImage(texture, m_x, m_y, 64.0f * 3.0f, 64.0f * 3.0f, opacity);
    } else {
        const int index = static_cast<int>(m_frame) % 8 + 1;
        renderer.DrawImage(L"Assets\\3stage\\Idle" + std::to_wstring(index) + L".png", m_x, m_y, 96.0f * 3.0f, 64.0f * 3.0f, opacity);
    }
}

void Boss::DrawHp(Renderer& renderer) const {
    const float percent = m_maxHp > 0 ? static_cast<float>(m_hp) / static_cast<float>(m_maxHp) : 0.0f;
    const float frameWidth = 512.0f;
    const float barWidth = frameWidth * std::clamp(percent, 0.0f, 1.0f);

    renderer.DrawImage(L"Assets\\bossui\\Enemy Health Frame.png", CanvasWidth * 0.5f, 24.0f, frameWidth, 16.0f);
    renderer.DrawImageClip(L"Assets\\bossui\\EnemyHealthBar.png", 0.0f, 0.0f, 288.0f, 16.0f,
        CanvasWidth * 0.5f - (frameWidth - barWidth) * 0.5f, 24.0f, barWidth, 16.0f);
}

