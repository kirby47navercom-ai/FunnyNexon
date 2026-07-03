#pragma once

#include "Patterns.h"

#include <filesystem>
#include <string>
#include <vector>

struct GesturePoint {
    double x = 0.0;
    double y = 0.0;
    int strokeId = 0;
    int intX = 0;
    int intY = 0;
};

struct GestureResult {
    PatternId id = PatternId::Unknown;
    std::wstring name;
    double score = 0.0;
    double milliseconds = 0.0;
};

class GestureRecognizer {
public:
    bool LoadFromFolder(const std::filesystem::path& folder);
    GestureResult Recognize(const std::vector<GesturePoint>& points) const;
    bool IsLoaded() const { return !m_templates.empty(); }

public:
    struct PointCloud {
        PatternId id = PatternId::Unknown;
        std::wstring name;
        std::vector<GesturePoint> points;
        std::vector<std::vector<int>> lut;
    };

private:
    bool LoadXmlFile(const std::filesystem::path& path);

private:
    std::vector<PointCloud> m_templates;
};
