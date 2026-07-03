#pragma once

#include <memory>

class GameApp;
class Renderer;

enum class SceneKind {
    Logo,
    Title,
    Home,
    Option,
    Shop,
    Confirm,
    Battle,
    Result,
    Ending
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual void OnEnter(GameApp& app) { (void)app; }
    virtual void OnExit(GameApp& app) { (void)app; }
    virtual void Update(GameApp& app, float deltaTime) = 0;
    virtual void Render(GameApp& app, Renderer& renderer) = 0;
};

std::unique_ptr<Scene> CreateScene(SceneKind kind, int value = 0, bool flag = false);

