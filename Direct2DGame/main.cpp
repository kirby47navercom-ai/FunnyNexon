#include "GameApp.h"

#include <Windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    GameApp app;

    HRESULT result = app.Initialize(instance, showCommand);
    if (FAILED(result)) {
        MessageBox(nullptr, L"Direct2D 게임 초기화에 실패했다.", L"Direct2DGame", MB_OK | MB_ICONERROR);
        return -1;
    }

    return app.Run();
}

