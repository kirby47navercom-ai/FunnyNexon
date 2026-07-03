#include "ResourcePath.h"

#include <Windows.h>
#include <vector>

namespace {
std::filesystem::path ModuleDirectory() {
    wchar_t buffer[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

void AddParents(std::vector<std::filesystem::path>& roots, std::filesystem::path path) {
    for (int i = 0; i < 6 && !path.empty(); ++i) {
        roots.push_back(path);
        path = path.parent_path();
    }
}
}

std::filesystem::path AssetPath(const std::wstring& relativePath) {
    std::vector<std::filesystem::path> roots;
    AddParents(roots, std::filesystem::current_path());
    AddParents(roots, ModuleDirectory());

    for (const auto& root : roots) {
        std::filesystem::path candidate = root / relativePath;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return std::filesystem::current_path() / relativePath;
}

