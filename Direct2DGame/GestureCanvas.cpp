#include "GestureCanvas.h"

#include "Constants.h"
#include "Input.h"
#include "Renderer.h"
#include "ResourcePath.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <thread>

namespace {
struct GestureTemplateCache {
    std::mutex mutex;
    std::shared_ptr<GestureRecognizer> recognizer;
    bool started = false;
    bool ready = false;
    bool loaded = false;
};

GestureTemplateCache& SharedTemplateCache() {
    static GestureTemplateCache cache;
    return cache;
}
}

void GestureCanvas::Reset() {
    m_points.clear();
    m_lastResult.reset();
    m_drawing = false;
    m_strokeId = 0;
    m_openAmount = 0.0f;
}

void GestureCanvas::LoadTemplates() {
    if (m_loaded) {
        return;
    }

    GestureTemplateCache& cache = SharedTemplateCache();
    bool shouldStart = false;

    {
        std::lock_guard<std::mutex> lock(cache.mutex);
        if (cache.ready) {
            m_recognizer = cache.recognizer;
            m_loaded = cache.loaded;
            return;
        }

        if (!cache.started) {
            cache.started = true;
            shouldStart = true;
        }
    }

    if (!shouldStart) {
        return;
    }

    std::thread([] {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

        auto recognizer = std::make_shared<GestureRecognizer>();
        const bool loaded = recognizer->LoadFromFolder(AssetPath(L"Assets\\NewGestures"));

        GestureTemplateCache& workerCache = SharedTemplateCache();
        std::lock_guard<std::mutex> lock(workerCache.mutex);
        workerCache.recognizer = loaded ? recognizer : nullptr;
        workerCache.loaded = loaded;
        workerCache.ready = true;
    }).detach();
}

std::optional<GestureResult> GestureCanvas::Update(const InputState& input, float deltaTime) {
    const bool wantsOpen = input.IsKeyDown('F');
    if (wantsOpen) {
        LoadTemplates();
    }

    const float direction = wantsOpen ? 1.0f : -1.0f;
    m_openAmount = std::clamp(m_openAmount + direction * deltaTime * 5.0f, 0.0f, 1.0f);

    if (!wantsOpen) {
        m_points.clear();
        m_lastResult.reset();
        m_drawing = false;
        return std::nullopt;
    }

    if (!IsReadyToDraw()) {
        return std::nullopt;
    }

    if (input.WasMousePressed()) {
        m_points.clear();
        m_lastResult.reset();
        m_drawing = true;
        ++m_strokeId;
        m_points.push_back({ input.MouseX(), input.MouseY(), m_strokeId });
    } else if (input.IsMouseDown() && m_drawing) {
        GesturePoint p { input.MouseX(), input.MouseY(), m_strokeId };
        if (m_points.empty() || PointDistance(m_points.back(), p) > 2.0) {
            m_points.push_back(p);
        }
    } else if (input.WasMouseReleased() && m_drawing) {
        m_drawing = false;
        if (m_points.size() > 10 && m_loaded && m_recognizer && m_recognizer->IsLoaded()) {
            m_lastResult = m_recognizer->Recognize(m_points);
            m_points.clear();
            if (m_lastResult->score >= 0.20 && m_lastResult->id != PatternId::Unknown) {
                return m_lastResult;
            }
        }
        m_points.clear();
    }

    return std::nullopt;
}

void GestureCanvas::Draw(Renderer& renderer) const {
    if (m_openAmount <= 0.0f) {
        return;
    }

    const float panelY = CanvasHeight + CanvasHeight * 0.5f - CanvasHeight * m_openAmount;
    const float checkY = CanvasHeight - 43.0f - 90.0f * (1.0f - m_openAmount);

    renderer.FillRect(0.0f, 0.0f, CanvasWidth, CanvasHeight, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.35f * m_openAmount));
    renderer.DrawImage(L"Assets\\Canvas\\1.png", CanvasWidth * 0.5f, panelY, CanvasWidth, CanvasHeight, 0.95f);
    renderer.DrawImageClip(L"Assets\\Canvas\\2.png", 0.0f, 0.0f, 825.0f, 216.0f,
        CanvasWidth * 0.5f, checkY, 825.0f * 0.4f, 216.0f * 0.4f, false, m_openAmount);

    for (size_t i = 1; i < m_points.size(); ++i) {
        renderer.DrawLine(static_cast<float>(m_points[i - 1].x), CanvasHeight - static_cast<float>(m_points[i - 1].y),
            static_cast<float>(m_points[i].x), CanvasHeight - static_cast<float>(m_points[i].y),
            D2D1::ColorF(D2D1::ColorF::Black), 4.0f);
    }

    if (m_lastResult) {
        const std::wstring text = (m_lastResult->score >= 0.20 && m_lastResult->id != PatternId::Unknown)
            ? L"인식 결과: " + m_lastResult->name
            : L"인식 실패";
        renderer.DrawTextPico(text, 18.0f, CanvasHeight - 118.0f, 360.0f, 34.0f, 26.0f,
            D2D1::ColorF(D2D1::ColorF::Black), DWRITE_TEXT_ALIGNMENT_LEADING);
    }
}

double GestureCanvas::PointDistance(const GesturePoint& a, const GesturePoint& b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}
