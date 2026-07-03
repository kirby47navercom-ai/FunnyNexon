#pragma once

#include <array>

class InputState {
public:
    void BeginFrame();

    void OnKeyDown(unsigned int key);
    void OnKeyUp(unsigned int key);
    void OnMouseMove(float x, float y);
    void OnMouseDown(float x, float y);
    void OnMouseUp(float x, float y);

    bool IsKeyDown(unsigned int key) const;
    bool WasKeyPressed(unsigned int key) const;

    bool IsMouseDown() const { return m_mouseDown; }
    bool WasMousePressed() const { return m_mousePressed; }
    bool WasMouseReleased() const { return m_mouseReleased; }

    float MouseX() const { return m_mouseX; }
    float MouseY() const { return m_mouseY; }
    float MousePicoY() const;

private:
    std::array<bool, 256> m_keys {};
    std::array<bool, 256> m_pressed {};

    bool m_mouseDown = false;
    bool m_mousePressed = false;
    bool m_mouseReleased = false;
    float m_mouseX = 0.0f;
    float m_mouseY = 0.0f;
};

