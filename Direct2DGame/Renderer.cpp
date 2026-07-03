#include "Renderer.h"

#include "Constants.h"
#include "ResourcePath.h"

#include <algorithm>

using Microsoft::WRL::ComPtr;

HRESULT Renderer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        return result;
    }

    result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_wicFactory.ReleaseAndGetAddressOf()));
    if (FAILED(result)) {
        return result;
    }

    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dwriteFactory.ReleaseAndGetAddressOf()));
    if (FAILED(result)) {
        return result;
    }

    return CreateDeviceResources();
}

HRESULT Renderer::CreateDeviceResources() {
    if (m_renderTarget) {
        return S_OK;
    }

    RECT rect = {};
    GetClientRect(m_hwnd, &rect);
    m_width = std::max<UINT>(1, rect.right - rect.left);
    m_height = std::max<UINT>(1, rect.bottom - rect.top);

    return m_d2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hwnd, D2D1::SizeU(m_width, m_height)),
        m_renderTarget.ReleaseAndGetAddressOf()
    );
}

void Renderer::Resize(UINT width, UINT height) {
    m_width = std::max<UINT>(1, width);
    m_height = std::max<UINT>(1, height);

    if (m_renderTarget) {
        m_renderTarget->Resize(D2D1::SizeU(m_width, m_height));
    }
}

HRESULT Renderer::BeginFrame() {
    HRESULT result = CreateDeviceResources();
    if (FAILED(result)) {
        return result;
    }

    m_renderTarget->BeginDraw();

    // 실제 창 크기와 상관없이 원본 게임 좌표 1280x720을 그대로 쓰기 위한 스케일이다.
    const float scaleX = static_cast<float>(m_width) / CanvasWidth;
    const float scaleY = static_cast<float>(m_height) / CanvasHeight;
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Scale(scaleX, scaleY));

    return S_OK;
}

HRESULT Renderer::EndFrame() {
    HRESULT result = m_renderTarget->EndDraw();
    if (result == D2DERR_RECREATE_TARGET) {
        m_textures.clear();
        m_renderTarget.Reset();
        return S_OK;
    }
    return result;
}

void Renderer::Clear(const D2D1_COLOR_F& color) {
    m_renderTarget->Clear(color);
}

Texture* Renderer::GetTexture(const std::wstring& path) {
    auto found = m_textures.find(path);
    if (found != m_textures.end()) {
        return found->second.bitmap ? &found->second : nullptr;
    }

    Texture texture;
    if (FAILED(LoadTexture(path, texture))) {
        m_textures.emplace(path, Texture {});
        return nullptr;
    }

    auto [inserted, _] = m_textures.emplace(path, texture);
    return &inserted->second;
}

HRESULT Renderer::LoadTexture(const std::wstring& path, Texture& texture) {
    const std::filesystem::path realPath = AssetPath(path);

    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT result = m_wicFactory->CreateDecoderFromFilename(realPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        return result;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    result = decoder->GetFrame(0, frame.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        return result;
    }

    UINT width = 0;
    UINT height = 0;
    frame->GetSize(&width, &height);

    ComPtr<IWICFormatConverter> converter;
    result = m_wicFactory->CreateFormatConverter(converter.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        return result;
    }

    result = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
    if (FAILED(result)) {
        return result;
    }

    result = m_renderTarget->CreateBitmapFromWicBitmap(converter.Get(), nullptr, texture.bitmap.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        return result;
    }

    texture.width = static_cast<float>(width);
    texture.height = static_cast<float>(height);
    return S_OK;
}

void Renderer::DrawImage(const std::wstring& path, float centerX, float centerY, float width, float height, float opacity) {
    Texture* texture = GetTexture(path);
    if (!texture) {
        FillRect(centerX - 80.0f, centerY - 24.0f, centerX + 80.0f, centerY + 24.0f, D2D1::ColorF(0.6f, 0.0f, 0.05f), 0.8f);
        DrawTextPico(L"missing", centerX - 60.0f, centerY + 10.0f, 120.0f, 24.0f, 16.0f, D2D1::ColorF(D2D1::ColorF::White), DWRITE_TEXT_ALIGNMENT_CENTER);
        return;
    }

    if (width <= 0.0f) {
        width = texture->width;
    }
    if (height <= 0.0f) {
        height = texture->height;
    }

    m_renderTarget->DrawBitmap(texture->bitmap.Get(), CenterRect(centerX, centerY, width, height), opacity);
}

void Renderer::DrawImageClip(const std::wstring& path, float srcLeft, float srcBottom, float srcWidth, float srcHeight,
    float centerX, float centerY, float drawWidth, float drawHeight, bool flipX, float opacity) {
    Texture* texture = GetTexture(path);
    if (!texture) {
        FillRect(centerX - drawWidth * 0.5f, centerY - drawHeight * 0.5f, centerX + drawWidth * 0.5f, centerY + drawHeight * 0.5f, D2D1::ColorF(0.6f, 0.0f, 0.05f), 0.8f);
        return;
    }

    const float srcTop = texture->height - srcBottom - srcHeight;
    const D2D1_RECT_F source = D2D1::RectF(srcLeft, srcTop, srcLeft + srcWidth, srcTop + srcHeight);
    const D2D1_RECT_F dest = CenterRect(centerX, centerY, drawWidth, drawHeight);

    if (!flipX) {
        m_renderTarget->DrawBitmap(texture->bitmap.Get(), dest, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
        return;
    }

    D2D1_MATRIX_3X2_F oldTransform = {};
    m_renderTarget->GetTransform(&oldTransform);
    const D2D1_MATRIX_3X2_F flip = D2D1::Matrix3x2F::Scale(-1.0f, 1.0f, D2D1::Point2F(centerX, CanvasHeight - centerY));
    m_renderTarget->SetTransform(flip * oldTransform);
    m_renderTarget->DrawBitmap(texture->bitmap.Get(), dest, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
    m_renderTarget->SetTransform(oldTransform);
}

void Renderer::FillRect(float left, float bottom, float right, float top, const D2D1_COLOR_F& color, float opacity) {
    ComPtr<ID2D1SolidColorBrush> brush;
    D2D1_COLOR_F actual = color;
    actual.a *= opacity;
    m_renderTarget->CreateSolidColorBrush(actual, brush.ReleaseAndGetAddressOf());
    m_renderTarget->FillRectangle(ToD2DRect(left, bottom, right, top), brush.Get());
}

void Renderer::DrawRect(float left, float bottom, float right, float top, const D2D1_COLOR_F& color, float strokeWidth) {
    ComPtr<ID2D1SolidColorBrush> brush;
    m_renderTarget->CreateSolidColorBrush(color, brush.ReleaseAndGetAddressOf());
    m_renderTarget->DrawRectangle(ToD2DRect(left, bottom, right, top), brush.Get(), strokeWidth);
}

void Renderer::FillEllipse(float centerX, float centerY, float radiusX, float radiusY, const D2D1_COLOR_F& color, float opacity) {
    ComPtr<ID2D1SolidColorBrush> brush;
    D2D1_COLOR_F actual = color;
    actual.a *= opacity;
    m_renderTarget->CreateSolidColorBrush(actual, brush.ReleaseAndGetAddressOf());
    m_renderTarget->FillEllipse(D2D1::Ellipse(ToD2DPoint(centerX, centerY), radiusX, radiusY), brush.Get());
}

void Renderer::DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth) {
    ComPtr<ID2D1SolidColorBrush> brush;
    m_renderTarget->CreateSolidColorBrush(color, brush.ReleaseAndGetAddressOf());
    m_renderTarget->DrawLine(ToD2DPoint(x1, y1), ToD2DPoint(x2, y2), brush.Get(), strokeWidth);
}

void Renderer::DrawTextPico(const std::wstring& text, float left, float top, float width, float height, float fontSize,
    const D2D1_COLOR_F& color, DWRITE_TEXT_ALIGNMENT alignment) {
    ComPtr<IDWriteTextFormat> format;
    HRESULT result = m_dwriteFactory->CreateTextFormat(L"Malgun Gothic", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"ko-kr", format.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        return;
    }

    format->SetTextAlignment(alignment);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    ComPtr<ID2D1SolidColorBrush> brush;
    m_renderTarget->CreateSolidColorBrush(color, brush.ReleaseAndGetAddressOf());
    const D2D1_RECT_F rect = D2D1::RectF(left, CanvasHeight - top, left + width, CanvasHeight - top + height);
    m_renderTarget->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), format.Get(), rect, brush.Get());
}

D2D1_RECT_F Renderer::ToD2DRect(float left, float bottom, float right, float top) const {
    return D2D1::RectF(left, CanvasHeight - top, right, CanvasHeight - bottom);
}

D2D1_RECT_F Renderer::CenterRect(float centerX, float centerY, float width, float height) const {
    return ToD2DRect(centerX - width * 0.5f, centerY - height * 0.5f, centerX + width * 0.5f, centerY + height * 0.5f);
}

D2D1_POINT_2F Renderer::ToD2DPoint(float x, float y) const {
    return D2D1::Point2F(x, CanvasHeight - y);
}

