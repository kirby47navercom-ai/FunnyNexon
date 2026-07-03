#include "Patterns.h"

#include "Renderer.h"

#include <array>
#include <unordered_map>

namespace {
constexpr std::array<PatternInfo, 17> PatternTable = { {
    { PatternId::Width, L"가로선", L"Assets\\Pattern\\1.png" },
    { PatternId::Height, L"세로선", L"Assets\\Pattern\\2.png" },
    { PatternId::FoxEar, L"여우귀", L"Assets\\Pattern\\3.png" },
    { PatternId::Victory, L"브이", L"Assets\\Pattern\\4.png" },
    { PatternId::Thunder, L"번개", L"Assets\\Pattern\\5.png" },
    { PatternId::Night, L"N", L"Assets\\Pattern\\6.png" },
    { PatternId::Star, L"별", L"Assets\\Pattern\\7.png" },
    { PatternId::Zzz, L"Z", L"Assets\\Pattern\\8.png" },
    { PatternId::Diamond, L"다이아몬드", L"Assets\\Pattern\\9.png" },
    { PatternId::Square, L"네모", L"Assets\\Pattern\\10.png" },
    { PatternId::Triangle, L"세모", L"Assets\\Pattern\\11.png" },
    { PatternId::Black1, L"검정1", L"Assets\\Pattern\\12.png" },
    { PatternId::Black2, L"검정2", L"Assets\\Pattern\\13.png" },
    { PatternId::Black3, L"검정3", L"Assets\\Pattern\\14.png" },
    { PatternId::Black4, L"검정4", L"Assets\\Pattern\\15.png" },
    { PatternId::Black5, L"검정5", L"Assets\\Pattern\\16.png" },
    { PatternId::Heart, L"하트", L"" }
} };

const PatternInfo UnknownInfo { PatternId::Unknown, L"인식 실패", L"" };
}

const PatternInfo& GetPatternInfo(PatternId id) {
    const int index = static_cast<int>(id);
    if (index < 0 || index >= static_cast<int>(PatternTable.size())) {
        return UnknownInfo;
    }
    return PatternTable[static_cast<size_t>(index)];
}

PatternId PatternIdFromName(const std::wstring& name) {
    static const std::unordered_map<std::wstring, PatternId> aliases {
        { L"가로선", PatternId::Width },
        { L"세로선", PatternId::Height },
        { L"여우귀", PatternId::FoxEar },
        { L"브이", PatternId::Victory },
        { L"번개", PatternId::Thunder },
        { L"N", PatternId::Night },
        { L"별", PatternId::Star },
        { L"Z", PatternId::Zzz },
        { L"다이아몬드", PatternId::Diamond },
        { L"네모", PatternId::Square },
        { L"세모", PatternId::Triangle },
        { L"검정1", PatternId::Black1 },
        { L"검정2", PatternId::Black2 },
        { L"검정3", PatternId::Black3 },
        { L"검정4", PatternId::Black4 },
        { L"검정5", PatternId::Black5 },
        { L"하트", PatternId::Heart }
    };

    const auto found = aliases.find(name);
    if (found != aliases.end()) {
        return found->second;
    }
    return PatternId::Unknown;
}

PatternId RandomPattern(std::mt19937& rng, bool normalMonsterOnly) {
    const int last = normalMonsterOnly ? static_cast<int>(PatternId::Triangle) : static_cast<int>(PatternId::Black5);
    std::uniform_int_distribution<int> pick(0, last);
    return static_cast<PatternId>(pick(rng));
}

bool IsDamagePattern(PatternId id) {
    const int value = static_cast<int>(id);
    return value >= static_cast<int>(PatternId::Width) && value <= static_cast<int>(PatternId::Black5);
}

void DrawPattern(Renderer& renderer, PatternId id, float x, float y, float scale) {
    const PatternInfo& info = GetPatternInfo(id);
    if (id == PatternId::Heart) {
        renderer.DrawTextPico(L"하트", x - 42.0f, y + 12.0f, 84.0f, 28.0f, 22.0f,
            D2D1::ColorF(0.96f, 0.20f, 0.35f), DWRITE_TEXT_ALIGNMENT_CENTER);
        return;
    }

    if (info.imagePath[0] == L'\0') {
        return;
    }

    renderer.DrawImage(info.imagePath, x, y, 128.0f * scale, 128.0f * scale);
}
