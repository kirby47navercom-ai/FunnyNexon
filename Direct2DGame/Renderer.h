#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <string>
#include <unordered_map>

struct Texture {
    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    float width = 0.0f;
    float height = 0.0f;
};

class Renderer {
public:
    HRESULT Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);

    HRESULT BeginFrame();
    HRESULT EndFrame();
    void Clear(const D2D1_COLOR_F& color);

    void DrawImage(const std::wstring& path, float centerX, float centerY, float width = -1.0f, float height = -1.0f, float opacity = 1.0f);
    void DrawImageClip(const std::wstring& path, float srcLeft, float srcBottom, float srcWidth, float srcHeight,
        float centerX, float centerY, float drawWidth, float drawHeight, bool flipX = false, float opacity = 1.0f);

    void FillRect(float left, float bottom, float right, float top, const D2D1_COLOR_F& color, float opacity = 1.0f);
    void DrawRect(float left, float bottom, float right, float top, const D2D1_COLOR_F& color, float strokeWidth = 1.0f);
    void FillEllipse(float centerX, float centerY, float radiusX, float radiusY, const D2D1_COLOR_F& color, float opacity = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth = 2.0f);
    void DrawTextPico(const std::wstring& text, float left, float top, float width, float height, float fontSize,
        const D2D1_COLOR_F& color, DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING);

private:
    HRESULT CreateDeviceResources();
    Texture* GetTexture(const std::wstring& path);
    HRESULT LoadTexture(const std::wstring& path, Texture& texture);

    D2D1_RECT_F ToD2DRect(float left, float bottom, float right, float top) const;
    D2D1_RECT_F CenterRect(float centerX, float centerY, float width, float height) const;
    D2D1_POINT_2F ToD2DPoint(float x, float y) const;

private:
    HWND m_hwnd = nullptr;
    UINT m_width = 1;
    UINT m_height = 1;

    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;

    std::unordered_map<std::wstring, Texture> m_textures;
};

