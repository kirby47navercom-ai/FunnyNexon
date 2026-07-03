#pragma once

#include <algorithm>
#include <cmath>

struct RectF {
    float left = 0.0f;
    float bottom = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
};

inline RectF CenterBox(float x, float y, float width, float height) {
    return { x - width * 0.5f, y - height * 0.5f, x + width * 0.5f, y + height * 0.5f };
}

inline bool Intersects(const RectF& a, const RectF& b) {
    return a.left <= b.right && a.right >= b.left && a.bottom <= b.top && a.top >= b.bottom;
}

inline bool CircleIntersects(const RectF& box, float x, float y, float radius) {
    const float closestX = std::clamp(x, box.left, box.right);
    const float closestY = std::clamp(y, box.bottom, box.top);
    const float dx = x - closestX;
    const float dy = y - closestY;
    return dx * dx + dy * dy <= radius * radius;
}

inline float Distance(float ax, float ay, float bx, float by) {
    const float dx = bx - ax;
    const float dy = by - ay;
    return std::sqrt(dx * dx + dy * dy);
}

inline void MoveToward(float& x, float& y, float targetX, float targetY, float speed, float deltaTime) {
    const float d = std::max(1.0f, Distance(x, y, targetX, targetY));
    x += (targetX - x) * speed * deltaTime / d;
    y += (targetY - y) * speed * deltaTime / d;
}
