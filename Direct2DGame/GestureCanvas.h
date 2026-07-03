#pragma once

#include "GestureRecognizer.h"

#include <memory>
#include <optional>
#include <vector>

class InputState;
class Renderer;

class GestureCanvas {
public:
    void Reset();
    void LoadTemplates();
    std::optional<GestureResult> Update(const InputState& input, float deltaTime);
    void Draw(Renderer& renderer) const;

    bool IsOpen() const { return m_openAmount > 0.05f; }
    bool IsReadyToDraw() const { return m_openAmount >= 0.98f; }

private:
    static double PointDistance(const GesturePoint& a, const GesturePoint& b);

private:
    std::shared_ptr<const GestureRecognizer> m_recognizer;
    std::vector<GesturePoint> m_points;
    std::optional<GestureResult> m_lastResult;
    bool m_loaded = false;
    bool m_drawing = false;
    int m_strokeId = 0;
    float m_openAmount = 0.0f;
};
