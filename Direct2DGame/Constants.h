#pragma once

// 원본 pico2d 프로젝트가 사용한 화면 좌표계다.
// Direct2D 창 크기가 달라도 게임 내부 좌표는 1280x720으로 유지한다.
constexpr float CanvasWidth = 1280.0f;
constexpr float CanvasHeight = 720.0f;

constexpr int WindowClientWidth = 1280;
constexpr int WindowClientHeight = 720;
