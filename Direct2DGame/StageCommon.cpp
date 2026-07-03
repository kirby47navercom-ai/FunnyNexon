#include "StageCommon.h"

#include "Constants.h"
#include "Player.h"
#include "Renderer.h"

#include <algorithm>

namespace stages {

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

std::wstring NumberedAsset(const std::wstring& prefix, int frame, const std::wstring& suffix) {
    return prefix + std::to_wstring(frame) + suffix;
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

void DrawStage3Backdrop(Renderer& renderer) {
    renderer.DrawImage(L"Assets\\3stage\\Fox BackGround 3.png", CanvasWidth * 0.5f, 410.0f, 800.0f * 3.0f, 480.0f * 3.0f);
    renderer.DrawImage(L"Assets\\3stage\\Fox BackGround 2.png", CanvasWidth * 0.5f, 410.0f, 800.0f * 3.0f, 480.0f * 3.0f);
    renderer.DrawImage(L"Assets\\3stage\\Fox BackGround 1.png", CanvasWidth * 0.5f, 410.0f, 800.0f * 2.0f, 480.0f * 2.0f);
    renderer.DrawImage(L"Assets\\3stage\\Fox Platform.png", CanvasWidth * 0.5f, 120.0f, 752.0f * 2.0f, 128.0f * 2.0f);
}

} // namespace stages
