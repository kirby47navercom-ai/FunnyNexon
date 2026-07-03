#pragma once

#include <filesystem>
#include <string>

// 실행 위치가 Rider/Visual Studio/exe 직접 실행 중 어디든 Assets 폴더를 찾기 위한 함수다.
std::filesystem::path AssetPath(const std::wstring& relativePath);

