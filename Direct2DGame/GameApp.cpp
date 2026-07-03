#include "GameApp.h"

#include "Constants.h"

#include <Windowsx.h>
#include <algorithm>
#include <chrono>

GameApp::GameApp() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
}

GameApp::~GameApp() {
    CoUninitialize();
}

HRESULT GameApp::Initialize(HINSTANCE instance, int showCommand) {
    const wchar_t className[] = L"Direct2DGameWindowClass";

    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = GameApp::WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClass(&windowClass)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            return HRESULT_FROM_WIN32(error);
        }
    }

    RECT rect = { 0, 0, WindowClientWidth, WindowClientHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowEx(
        0,
        className,
        L"Direct2DGame",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        this
    );

    if (!m_hwnd) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT result = m_renderer.Initialize(m_hwnd);
    if (FAILED(result)) {
        return result;
    }

    ChangeScene(SceneKind::Logo);

    ShowWindow(m_hwnd, showCommand);
    UpdateWindow(m_hwnd);
    return S_OK;
}

int GameApp::Run() {
    m_running = true;

    using Clock = std::chrono::steady_clock;
    constexpr auto TargetFrameTime = std::chrono::duration<float>(1.0f / 60.0f);
    auto previous = Clock::now();

    while (m_running) {
        const auto frameStart = Clock::now();
        m_input.BeginFrame();

        MSG message = {};
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                m_running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        if (!m_running) {
            break;
        }

        const auto now = Clock::now();
        float deltaTime = std::chrono::duration<float>(now - previous).count();
        previous = now;
        deltaTime = std::min(deltaTime, 0.05f);

        if (m_scene) {
            m_scene->Update(*this, deltaTime);
        }

        Render();

        const auto frameElapsed = Clock::now() - frameStart;
        if (frameElapsed < TargetFrameTime) {
            const auto rest = std::chrono::duration_cast<std::chrono::milliseconds>(TargetFrameTime - frameElapsed);
            if (rest.count() > 0) {
                Sleep(static_cast<DWORD>(rest.count()));
            }
        }
    }

    return 0;
}

void GameApp::ChangeScene(SceneKind kind, int value, bool flag) {
    if (m_scene) {
        m_scene->OnExit(*this);
    }

    m_scene = CreateScene(kind, value, flag);

    if (m_scene) {
        m_scene->OnEnter(*this);
    }
}

void GameApp::Quit() {
    m_running = false;
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

void GameApp::Render() {
    if (FAILED(m_renderer.BeginFrame())) {
        return;
    }

    m_renderer.Clear(D2D1::ColorF(0.04f, 0.04f, 0.05f));

    if (m_scene) {
        m_scene->Render(*this, m_renderer);
    }

    m_renderer.EndFrame();
}

LRESULT CALLBACK GameApp::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    GameApp* app = nullptr;

    if (message == WM_NCCREATE) {
        CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = static_cast<GameApp*>(create->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->m_hwnd = hwnd;
    } else {
        app = reinterpret_cast<GameApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (app) {
        return app->HandleMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT GameApp::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        m_renderer.Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            Quit();
            return 0;
        }
        m_input.OnKeyDown(static_cast<unsigned int>(wParam));
        return 0;

    case WM_KEYUP:
        m_input.OnKeyUp(static_cast<unsigned int>(wParam));
        return 0;

    case WM_MOUSEMOVE:
        UpdateMouseFromClient(lParam, false, false, true);
        return 0;

    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        UpdateMouseFromClient(lParam, true, false, false);
        return 0;

    case WM_LBUTTONUP:
        ReleaseCapture();
        UpdateMouseFromClient(lParam, false, true, false);
        return 0;

    case WM_CLOSE:
        Quit();
        return 0;

    case WM_DESTROY:
        m_running = false;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void GameApp::UpdateMouseFromClient(LPARAM lParam, bool down, bool up, bool move) {
    RECT rect = {};
    GetClientRect(m_hwnd, &rect);

    const float width = static_cast<float>(std::max(1L, rect.right - rect.left));
    const float height = static_cast<float>(std::max(1L, rect.bottom - rect.top));
    const float x = static_cast<float>(GET_X_LPARAM(lParam)) * CanvasWidth / width;
    const float y = static_cast<float>(GET_Y_LPARAM(lParam)) * CanvasHeight / height;

    if (down) {
        m_input.OnMouseDown(x, y);
    } else if (up) {
        m_input.OnMouseUp(x, y);
    } else if (move) {
        m_input.OnMouseMove(x, y);
    }
}
