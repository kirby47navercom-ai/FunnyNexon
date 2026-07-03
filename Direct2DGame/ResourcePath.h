#pragma once

#include <filesystem>
#include <string>

// Rider, Visual Studio, exe 직접 실행처럼 작업 디렉터리가 달라져도 Assets 폴더를 찾기 위한 함수다.
std::filesystem::path AssetPath(const std::wstring& relativePath);
