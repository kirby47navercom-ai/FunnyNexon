#pragma once

#include "GameState.h"
#include "Input.h"
#include "Renderer.h"
#include "Scenes.h"

#include <Windows.h>
#include <memory>

class GameApp {
public:
    GameApp();
    ~GameApp();

    HRESULT Initialize(HINSTANCE instance, int showCommand);
    int Run();
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void ChangeScene(SceneKind kind, int value = 0, bool flag = false);
    void Quit();

    GameState& State() { return m_state; }
    InputState& Input() { return m_input; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void UpdateMouseFromClient(LPARAM lParam, bool down, bool up, bool move);
    void Render();

private:
    HWND m_hwnd = nullptr;
    bool m_running = false;

    Renderer m_renderer;
    InputState m_input;
    GameState m_state;
    std::unique_ptr<Scene> m_scene;
};

