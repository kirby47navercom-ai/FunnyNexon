#include "Input.h"

#include "Constants.h"

void InputState::BeginFrame() {
    m_pressed.fill(false);
    m_mousePressed = false;
    m_mouseReleased = false;
}

void InputState::OnKeyDown(unsigned int key) {
    if (key < m_keys.size()) {
        if (!m_keys[key]) {
            m_pressed[key] = true;
        }
        m_keys[key] = true;
    }
}

void InputState::OnKeyUp(unsigned int key) {
    if (key < m_keys.size()) {
        m_keys[key] = false;
    }
}

void InputState::OnMouseMove(float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
}

void InputState::OnMouseDown(float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
    m_mouseDown = true;
    m_mousePressed = true;
}

void InputState::OnMouseUp(float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
    m_mouseDown = false;
    m_mouseReleased = true;
}

bool InputState::IsKeyDown(unsigned int key) const {
    return key < m_keys.size() && m_keys[key];
}

bool InputState::WasKeyPressed(unsigned int key) const {
    return key < m_pressed.size() && m_pressed[key];
}

float InputState::MousePicoY() const {
    // pico2d 좌표는 아래가 y=0이고, Win32 마우스 좌표는 위가 y=0이라 뒤집어 준다.
    return CanvasHeight - 1.0f - m_mouseY;
}
