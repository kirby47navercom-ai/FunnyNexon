#pragma once

#include "Geometry.h"
#include "GestureRecognizer.h"

#include <random>
#include <string>

class Player;
class Renderer;

namespace stages {

// 여러 스테이지에서 같이 쓰는 단순 투사체 데이터다.
// STL 컨테이너에 값 타입으로 보관해서 new/delete 없이 수명 관리를 끝낸다.
struct Projectile {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float radius = 16.0f;
    float frame = 0.0f;
    float life = 0.0f;
};

struct TimedHitBox {
    RectF box {};
    float frame = 0.0f;
    float prepare = 0.0f;
    float active = 0.0f;
    float elapsed = 0.0f;
    int kind = 0;
};

std::mt19937& Rng();
float RandomRange(float minValue, float maxValue);
int RandomInt(int minValue, int maxValue);

std::wstring NumberedAsset(const std::wstring& prefix, int frame, const std::wstring& suffix = L".png");

void DamagePlayerOnTouch(Player& player, const RectF& box);
void DrawBossHp(Renderer& renderer, int hp, int maxHp);
void DrawStage3Backdrop(Renderer& renderer);

} // namespace stages
