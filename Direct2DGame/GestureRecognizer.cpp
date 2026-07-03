#include "GestureRecognizer.h"

#include "ResourcePath.h"

#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cwctype>
#include <fstream>
#include <limits>
#include <sstream>

namespace {
constexpr int NumPoints = 32;
constexpr int MaxIntCoord = 1024;
constexpr int LutSize = 64;
constexpr double LutScaleFactor = static_cast<double>(MaxIntCoord) / static_cast<double>(LutSize);

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

std::wstring PrefixName(const std::filesystem::path& path) {
    std::wstring stem = path.stem().wstring();
    const size_t dash = stem.find(L'-');
    if (dash != std::wstring::npos) {
        stem = stem.substr(0, dash);
    }
    return stem;
}

bool ExtractAttributeDouble(const std::wstring& tag, const std::wstring& name, double& value) {
    size_t pos = tag.find(name);
    while (pos != std::wstring::npos) {
        const size_t afterName = pos + name.size();
        size_t cursor = afterName;
        while (cursor < tag.size() && iswspace(tag[cursor])) {
            ++cursor;
        }
        if (cursor < tag.size() && tag[cursor] == L'=') {
            ++cursor;
            while (cursor < tag.size() && iswspace(tag[cursor])) {
                ++cursor;
            }
            if (cursor < tag.size() && tag[cursor] == L'"') {
                const size_t begin = cursor + 1;
                const size_t end = tag.find(L'"', begin);
                if (end != std::wstring::npos) {
                    value = std::stod(tag.substr(begin, end - begin));
                    return true;
                }
            }
        }
        pos = tag.find(name, afterName);
    }
    return false;
}

double SqrDistance(const GesturePoint& a, const GesturePoint& b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return dx * dx + dy * dy;
}

double Distance(const GesturePoint& a, const GesturePoint& b) {
    return std::sqrt(SqrDistance(a, b));
}

double PathLength(const std::vector<GesturePoint>& points) {
    double length = 0.0;
    for (size_t i = 1; i < points.size(); ++i) {
        if (points[i].strokeId == points[i - 1].strokeId) {
            length += Distance(points[i - 1], points[i]);
        }
    }
    return length;
}

std::vector<GesturePoint> Resample(std::vector<GesturePoint> points, int count) {
    if (points.empty()) {
        return {};
    }

    const double interval = PathLength(points) / static_cast<double>(count - 1);
    if (interval <= 0.0) {
        return points;
    }

    double accumulated = 0.0;
    std::vector<GesturePoint> result;
    result.push_back(points.front());

    for (size_t i = 1; i < points.size();) {
        if (points[i].strokeId == points[i - 1].strokeId) {
            const double d = Distance(points[i - 1], points[i]);
            if (d <= 0.000001) {
                ++i;
            } else if ((accumulated + d) >= interval) {
                GesturePoint q;
                q.x = points[i - 1].x + ((interval - accumulated) / d) * (points[i].x - points[i - 1].x);
                q.y = points[i - 1].y + ((interval - accumulated) / d) * (points[i].y - points[i - 1].y);
                q.strokeId = points[i].strokeId;
                result.push_back(q);
                points.insert(points.begin() + static_cast<std::ptrdiff_t>(i), q);
                accumulated = 0.0;
            } else {
                accumulated += d;
                ++i;
            }
        } else {
            ++i;
        }
    }

    while (result.size() < static_cast<size_t>(count)) {
        result.push_back(points.back());
    }
    if (result.size() > static_cast<size_t>(count)) {
        result.resize(static_cast<size_t>(count));
    }
    return result;
}

std::vector<GesturePoint> Scale(const std::vector<GesturePoint>& points) {
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();

    for (const GesturePoint& p : points) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }

    const double size = std::max(1.0, std::max(maxX - minX, maxY - minY));
    std::vector<GesturePoint> result;
    result.reserve(points.size());
    for (const GesturePoint& p : points) {
        result.push_back({ (p.x - minX) / size, (p.y - minY) / size, p.strokeId });
    }
    return result;
}

GesturePoint Centroid(const std::vector<GesturePoint>& points) {
    double x = 0.0;
    double y = 0.0;
    for (const GesturePoint& p : points) {
        x += p.x;
        y += p.y;
    }
    return { x / static_cast<double>(points.size()), y / static_cast<double>(points.size()), 0 };
}

std::vector<GesturePoint> TranslateToOrigin(const std::vector<GesturePoint>& points) {
    const GesturePoint c = Centroid(points);
    std::vector<GesturePoint> result;
    result.reserve(points.size());
    for (const GesturePoint& p : points) {
        result.push_back({ p.x - c.x, p.y - c.y, p.strokeId });
    }
    return result;
}

std::vector<GesturePoint> MakeIntCoords(std::vector<GesturePoint> points) {
    for (GesturePoint& p : points) {
        p.intX = static_cast<int>(std::llround((p.x + 1.0) / 2.0 * (MaxIntCoord - 1)));
        p.intY = static_cast<int>(std::llround((p.y + 1.0) / 2.0 * (MaxIntCoord - 1)));
        p.intX = std::clamp(p.intX, 0, MaxIntCoord - 1);
        p.intY = std::clamp(p.intY, 0, MaxIntCoord - 1);
    }
    return points;
}

std::vector<std::vector<int>> ComputeLut(const std::vector<GesturePoint>& points) {
    std::vector<std::vector<int>> lut(LutSize, std::vector<int>(LutSize, 0));
    for (int x = 0; x < LutSize; ++x) {
        for (int y = 0; y < LutSize; ++y) {
            int bestIndex = 0;
            double best = std::numeric_limits<double>::infinity();
            for (size_t i = 0; i < points.size(); ++i) {
                const int row = static_cast<int>(std::llround(points[i].intX / LutScaleFactor));
                const int col = static_cast<int>(std::llround(points[i].intY / LutScaleFactor));
                const double dx = static_cast<double>(row - x);
                const double dy = static_cast<double>(col - y);
                const double d = dx * dx + dy * dy;
                if (d < best) {
                    best = d;
                    bestIndex = static_cast<int>(i);
                }
            }
            lut[x][y] = bestIndex;
        }
    }
    return lut;
}

GestureRecognizer::PointCloud BuildCloud(PatternId id, const std::wstring& name, const std::vector<GesturePoint>& source) {
    GestureRecognizer::PointCloud cloud;
    cloud.id = id;
    cloud.name = name;
    cloud.points = MakeIntCoords(TranslateToOrigin(Scale(Resample(source, NumPoints))));
    cloud.lut = ComputeLut(cloud.points);
    return cloud;
}

std::vector<double> ComputeLowerBound(const std::vector<GesturePoint>& points1, const std::vector<GesturePoint>& points2, int step,
    const std::vector<std::vector<int>>& lut) {
    const int n = static_cast<int>(points1.size());
    std::vector<double> lowerBound(static_cast<size_t>(n / step + 1), 0.0);
    std::vector<double> sat(static_cast<size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        const int x = std::clamp(static_cast<int>(std::llround(points1[static_cast<size_t>(i)].intX / LutScaleFactor)), 0, LutSize - 1);
        const int y = std::clamp(static_cast<int>(std::llround(points1[static_cast<size_t>(i)].intY / LutScaleFactor)), 0, LutSize - 1);
        const int index = lut[static_cast<size_t>(x)][static_cast<size_t>(y)];
        const double d = SqrDistance(points1[static_cast<size_t>(i)], points2[static_cast<size_t>(index)]);
        sat[static_cast<size_t>(i)] = (i == 0) ? d : sat[static_cast<size_t>(i - 1)] + d;
        lowerBound[0] += static_cast<double>(n - i) * d;
    }

    for (int i = step; i < n; i += step) {
        const int j = i / step;
        lowerBound[static_cast<size_t>(j)] = lowerBound[0] + i * sat[static_cast<size_t>(n - 1)] - n * sat[static_cast<size_t>(i - 1)];
    }
    return lowerBound;
}

double CloudDistance(const std::vector<GesturePoint>& points1, const std::vector<GesturePoint>& points2, int start, double minSoFar) {
    const int n = static_cast<int>(points1.size());
    std::vector<int> unmatched(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        unmatched[static_cast<size_t>(i)] = i;
    }

    int i = start;
    int weight = n;
    double sum = 0.0;
    while (true) {
        int bestSlot = -1;
        double best = std::numeric_limits<double>::infinity();
        for (size_t slot = 0; slot < unmatched.size(); ++slot) {
            const double d = SqrDistance(points1[static_cast<size_t>(i)], points2[static_cast<size_t>(unmatched[slot])]);
            if (d < best) {
                best = d;
                bestSlot = static_cast<int>(slot);
            }
        }

        unmatched.erase(unmatched.begin() + bestSlot);
        sum += static_cast<double>(weight) * best;
        if (sum >= minSoFar) {
            return sum;
        }

        --weight;
        i = (i + 1) % n;
        if (i == start) {
            break;
        }
    }
    return sum;
}

double CloudMatch(const GestureRecognizer::PointCloud& candidate, const GestureRecognizer::PointCloud& templ, double minSoFar) {
    const int n = static_cast<int>(candidate.points.size());
    const int step = static_cast<int>(std::floor(std::sqrt(static_cast<double>(n))));
    const std::vector<double> lb1 = ComputeLowerBound(candidate.points, templ.points, step, templ.lut);
    const std::vector<double> lb2 = ComputeLowerBound(templ.points, candidate.points, step, candidate.lut);

    for (size_t i = 0; i < lb1.size() && i < lb2.size(); ++i) {
        const int j = static_cast<int>(i) * step;
        if (lb1[i] < minSoFar) {
            minSoFar = std::min(minSoFar, CloudDistance(candidate.points, templ.points, j, minSoFar));
        }
        if (lb2[i] < minSoFar) {
            minSoFar = std::min(minSoFar, CloudDistance(templ.points, candidate.points, j, minSoFar));
        }
    }
    return minSoFar;
}
}

bool GestureRecognizer::LoadFromFolder(const std::filesystem::path& folder) {
    m_templates.clear();

    std::filesystem::path realFolder = folder;
    if (!std::filesystem::exists(realFolder)) {
        realFolder = AssetPath(folder.wstring());
    }
    if (!std::filesystem::exists(realFolder)) {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(realFolder)) {
        if (entry.is_regular_file() && entry.path().extension() == L".xml") {
            LoadXmlFile(entry.path());
        }
    }
    return !m_templates.empty();
}

bool GestureRecognizer::LoadXmlFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::wstring xml = Utf8ToWide(buffer.str());

    const std::wstring name = PrefixName(path);
    const PatternId id = PatternIdFromName(name);
    if (id == PatternId::Unknown) {
        return false;
    }

    std::vector<GesturePoint> points;
    int strokeId = 0;
    size_t strokeBegin = 0;
    while ((strokeBegin = xml.find(L"<Stroke", strokeBegin)) != std::wstring::npos) {
        const size_t strokeOpenEnd = xml.find(L'>', strokeBegin);
        if (strokeOpenEnd == std::wstring::npos) {
            break;
        }
        const size_t strokeEnd = xml.find(L"</Stroke>", strokeOpenEnd);
        if (strokeEnd == std::wstring::npos) {
            break;
        }

        ++strokeId;
        size_t pointBegin = strokeOpenEnd + 1;
        while ((pointBegin = xml.find(L"<Point", pointBegin)) != std::wstring::npos && pointBegin < strokeEnd) {
            const size_t pointEnd = xml.find(L"/>", pointBegin);
            if (pointEnd == std::wstring::npos || pointEnd > strokeEnd) {
                break;
            }

            const std::wstring pointTag = xml.substr(pointBegin, pointEnd - pointBegin + 2);
            double x = 0.0;
            double y = 0.0;
            if (ExtractAttributeDouble(pointTag, L"X", x) && ExtractAttributeDouble(pointTag, L"Y", y)) {
                points.push_back({ x, y, strokeId });
            }
            pointBegin = pointEnd + 2;
        }
        strokeBegin = strokeEnd + 9;
    }

    if (points.size() < 2) {
        return false;
    }

    m_templates.push_back(BuildCloud(id, name, points));
    return true;
}

GestureResult GestureRecognizer::Recognize(const std::vector<GesturePoint>& points) const {
    const auto start = std::chrono::steady_clock::now();
    GestureResult result;
    if (points.size() < 2 || m_templates.empty()) {
        return result;
    }

    const PointCloud candidate = BuildCloud(PatternId::Unknown, L"", points);
    int bestIndex = -1;
    double best = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < m_templates.size(); ++i) {
        const double d = CloudMatch(candidate, m_templates[i], best);
        if (d < best) {
            best = d;
            bestIndex = static_cast<int>(i);
        }
    }

    const auto finish = std::chrono::steady_clock::now();
    result.milliseconds = std::chrono::duration<double, std::milli>(finish - start).count();
    if (bestIndex >= 0) {
        const PointCloud& templ = m_templates[static_cast<size_t>(bestIndex)];
        result.id = templ.id;
        result.name = templ.name;
        result.score = best > 1.0 ? 1.0 / best : 1.0;
    }
    return result;
}
