#pragma once

#include <random>
#include <string>
#include <vector>

class Renderer;

enum class PatternId {
    Unknown = -1,
    Width = 0,
    Height,
    FoxEar,
    Victory,
    Thunder,
    Night,
    Star,
    Zzz,
    Diamond,
    Square,
    Triangle,
    Black1,
    Black2,
    Black3,
    Black4,
    Black5,
    Heart
};

struct PatternInfo {
    PatternId id = PatternId::Unknown;
    const wchar_t* name = L"";
    const wchar_t* imagePath = L"";
};

const PatternInfo& GetPatternInfo(PatternId id);
PatternId PatternIdFromName(const std::wstring& name);
PatternId RandomPattern(std::mt19937& rng, bool normalMonsterOnly);
bool IsDamagePattern(PatternId id);
void DrawPattern(Renderer& renderer, PatternId id, float x, float y, float scale = 0.2f);

