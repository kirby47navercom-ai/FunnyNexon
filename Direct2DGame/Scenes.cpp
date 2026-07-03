#include "Scenes.h"

#include "Constants.h"
#include "GameApp.h"
#include "GestureCanvas.h"
#include "Input.h"
#include "Player.h"
#include "Renderer.h"
#include "Stages.h"

#include <algorithm>
#include <memory>
#include <string>

namespace {
bool Clicked(const InputState& input, float left, float bottom, float right, float top) {
    if (!input.WasMouseReleased()) {
        return false;
    }

    const float x = input.MouseX();
    const float y = input.MousePicoY();
    return left <= x && x <= right && bottom <= y && y <= top;
}

void DrawCoin(Renderer& renderer, int coin) {
    renderer.DrawImage(L"Assets\\배경\\coin.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
    renderer.DrawTextPico(L"X " + std::to_wstring(coin), 82.0f, 700.0f, 260.0f, 70.0f, 52.0f,
        D2D1::ColorF(D2D1::ColorF::Black));
}

void DrawStageFood(Renderer& renderer, int stage) {
    if (stage == 1) {
        renderer.DrawImage(L"Assets\\배경\\sugar.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
    } else if (stage == 2) {
        renderer.DrawImage(L"Assets\\배경\\water.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
    } else {
        renderer.DrawImage(L"Assets\\배경\\lemon.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
    }
}

void ApplyWeapon(GameState& state, int weapon) {
    state.selectedWeapon = weapon;
    if (weapon == 2) {
        state.playerMaxHp = 4;
        state.playerAttack = 20;
    } else if (weapon == 3) {
        state.playerMaxHp = 3;
        state.playerAttack = 30;
    } else {
        state.playerMaxHp = 3;
        state.playerAttack = 20;
    }
}

class LogoScene final : public Scene {
public:
    void OnEnter(GameApp&) override { m_timer = 0.0f; }

    void Update(GameApp& app, float deltaTime) override {
        m_timer += deltaTime;
        if (m_timer >= 2.0f || app.Input().WasMouseReleased()) {
            app.ChangeScene(SceneKind::Title);
        }
    }

    void Render(GameApp&, Renderer& renderer) override {
        renderer.DrawImage(L"Assets\\tuk_credit.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
    }

private:
    float m_timer = 0.0f;
};

class TitleScene final : public Scene {
public:
    void Update(GameApp& app, float) override {
        const InputState& input = app.Input();
        if (Clicked(input, 410.0f, 30.0f, 618.0f, 100.0f)) {
            app.ChangeScene(SceneKind::Home);
        } else if (Clicked(input, 660.0f, 30.0f, 868.0f, 100.0f)) {
            app.ChangeScene(SceneKind::Option, static_cast<int>(SceneKind::Title));
        }
    }

    void Render(GameApp&, Renderer& renderer) override {
        renderer.DrawImage(L"Assets\\배경\\main.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
    }
};

class HomeScene final : public Scene {
public:
    void Update(GameApp& app, float) override {
        GameState& state = app.State();
        if (state.AllStagesCleared() && !state.endingSeen) {
            state.endingSeen = true;
            app.ChangeScene(SceneKind::Ending);
            return;
        }

        const InputState& input = app.Input();
        if (Clicked(input, 15.0f, 130.0f, 305.0f, 545.0f)) {
            app.ChangeScene(SceneKind::Confirm, 1);
        } else if (Clicked(input, 500.0f, 130.0f, 790.0f, 545.0f)) {
            app.ChangeScene(SceneKind::Confirm, 2);
        } else if (Clicked(input, 965.0f, 130.0f, 1255.0f, 545.0f)) {
            app.ChangeScene(SceneKind::Confirm, 3);
        } else if (Clicked(input, 1145.0f, 585.0f, 1255.0f, 690.0f)) {
            app.ChangeScene(SceneKind::Option, static_cast<int>(SceneKind::Home));
        } else if (Clicked(input, 495.0f, 605.0f, 855.0f, 670.0f)) {
            app.ChangeScene(SceneKind::Shop);
        }
    }

    void Render(GameApp& app, Renderer& renderer) override {
        const GameState& state = app.State();
        renderer.DrawImage(L"Assets\\배경\\stage_memu.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);

        for (int i = 0; i < 3; ++i) {
            if (state.stageClear[i] == 1) {
                renderer.DrawImage(L"Assets\\배경\\normal" + std::to_wstring(i + 1) + L".png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
            } else if (state.stageClear[i] == 2) {
                renderer.DrawImage(L"Assets\\배경\\perfect" + std::to_wstring(i + 1) + L".png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
            }

            if (state.stageClear[i] > 0) {
                renderer.DrawImage(L"Assets\\배경\\clear" + std::to_wstring(i + 1) + L".png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
            }
        }
        DrawCoin(renderer, state.coin);
    }
};

class OptionScene final : public Scene {
public:
    explicit OptionScene(SceneKind previous) : m_previous(previous) {}

    void Update(GameApp& app, float) override {
        GameState& state = app.State();
        const InputState& input = app.Input();
        const float xs[5][2] = {
            {475.0f, 545.0f}, {585.0f, 655.0f}, {705.0f, 775.0f}, {830.0f, 905.0f}, {955.0f, 1025.0f}
        };

        for (int i = 0; i < 5; ++i) {
            if (Clicked(input, xs[i][0], 360.0f, xs[i][1], 430.0f)) {
                state.bgmVolume = i;
            }
            if (Clicked(input, xs[i][0], 190.0f, xs[i][1], 260.0f)) {
                state.effectVolume = i;
            }
        }

        if (Clicked(input, 1055.0f, 515.0f, 1185.0f, 640.0f)) {
            app.ChangeScene(m_previous);
        }
    }

    void Render(GameApp& app, Renderer& renderer) override {
        const GameState& state = app.State();
        renderer.DrawImage(L"Assets\\배경\\black_background.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        renderer.DrawImage(L"Assets\\배경\\option.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        renderer.DrawImage(L"Assets\\배경\\circle" + std::to_wstring(state.bgmVolume + 1) + L".png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        renderer.DrawImage(L"Assets\\배경\\circle" + std::to_wstring(state.effectVolume + 6) + L".png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
    }

private:
    SceneKind m_previous = SceneKind::Title;
};

class ShopScene final : public Scene {
public:
    void Update(GameApp& app, float) override {
        const InputState& input = app.Input();
        if (Clicked(input, 495.0f, 605.0f, 855.0f, 670.0f)) {
            app.ChangeScene(SceneKind::Home);
            return;
        }

        if (Clicked(input, 833.0f, 540.0f, 873.0f, 580.0f)) {
            m_talk = 1;
        } else if (Clicked(input, 1060.0f, 540.0f, 1100.0f, 580.0f)) {
            m_talk = 2;
        } else if (Clicked(input, 835.0f, 245.0f, 875.0f, 285.0f)) {
            m_talk = 3;
        } else if (Clicked(input, 1060.0f, 245.0f, 1100.0f, 285.0f)) {
            m_talk = 4;
        }

        TryWeapon(app, 1, 0, Clicked(input, 860.0f, 318.0f, 1010.0f, 390.0f));
        TryWeapon(app, 2, 1, Clicked(input, 1090.0f, 318.0f, 1240.0f, 390.0f));
        TryWeapon(app, 3, 2, Clicked(input, 860.0f, 28.0f, 1010.0f, 100.0f));
        TryWeapon(app, 4, 3, Clicked(input, 1090.0f, 28.0f, 1240.0f, 100.0f));
    }

    void Render(GameApp& app, Renderer& renderer) override {
        const GameState& state = app.State();
        renderer.DrawImage(L"Assets\\배경\\na25_background.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        renderer.DrawImage(m_talk > 0 ? L"Assets\\배경\\na2.png" : L"Assets\\배경\\na1.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        renderer.DrawImage(L"Assets\\배경\\inventory.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);

        if (state.selectedWeapon == 1) {
            renderer.DrawImage(L"Assets\\배경\\weapon1_on.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        }
        DrawWeapon(renderer, state, 2, L"Assets\\배경\\weapon2_on.png", L"Assets\\배경\\weapon2_buy.png");
        DrawWeapon(renderer, state, 3, L"Assets\\배경\\weapon3_on.png", L"Assets\\배경\\weapon3_buy.png");
        DrawWeapon(renderer, state, 4, L"Assets\\배경\\weapon4_on.png", L"Assets\\배경\\weapon4_buy.png");

        if (m_talk >= 1 && m_talk <= 4) {
            renderer.DrawImage(L"Assets\\배경\\talk" + std::to_wstring(m_talk) + L".png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        }
        DrawCoin(renderer, state.coin);
    }

private:
    void TryWeapon(GameApp& app, int weapon, int cost, bool clicked) {
        if (!clicked) {
            return;
        }

        GameState& state = app.State();
        const int index = weapon - 1;
        if (!state.weaponOwned[index]) {
            if (state.coin < cost) {
                return;
            }
            state.coin -= cost;
            state.weaponOwned[index] = true;
        }
        ApplyWeapon(state, weapon);
    }

    void DrawWeapon(Renderer& renderer, const GameState& state, int weapon, const std::wstring& onImage, const std::wstring& buyImage) {
        const int index = weapon - 1;
        if (!state.weaponOwned[index]) {
            renderer.DrawImage(buyImage, CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        } else if (state.selectedWeapon == weapon) {
            renderer.DrawImage(onImage, CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        }
    }

private:
    int m_talk = 0;
};

class ConfirmScene final : public Scene {
public:
    explicit ConfirmScene(int stage) : m_stage(stage) {}

    void Update(GameApp& app, float) override {
        const InputState& input = app.Input();
        if (Clicked(input, 240.0f, 230.0f, 530.0f, 360.0f)) {
            app.ChangeScene(SceneKind::Battle, m_stage);
        } else if (Clicked(input, 750.0f, 230.0f, 1040.0f, 360.0f)) {
            app.ChangeScene(SceneKind::Home);
        }
    }

    void Render(GameApp&, Renderer& renderer) override {
        renderer.DrawImage(L"Assets\\배경\\black_background.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        renderer.DrawImage(L"Assets\\배경\\choose.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        renderer.DrawImage(L"Assets\\배경\\stage" + std::to_wstring(m_stage) + L".png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
    }

private:
    int m_stage = 1;
};

class BattleScene final : public Scene {
public:
    explicit BattleScene(int stage) : m_stage(stage) {}

    void OnEnter(GameApp& app) override {
        m_player.Reset(app.State(), m_stage);
        m_gesture.Reset();
        m_gesture.LoadTemplates();
        m_stageController = CreateStageController(m_stage);
    }

    void Update(GameApp& app, float deltaTime) override {
        InputState& input = app.Input();
        const auto gesture = m_gesture.Update(input, deltaTime);
        m_player.Update(input, deltaTime, m_gesture.IsOpen());

        if (gesture && m_stageController) {
            m_stageController->ApplyGesture(gesture->id, m_player);
        }
        if (m_stageController) {
            m_stageController->Update(deltaTime, m_player);
        }

        if (m_stageController && m_stageController->IsCleared()) {
            app.State().lastBattlePerfect = (m_player.Hp() == m_player.MaxHp());
            app.ChangeScene(SceneKind::Result, m_stage, true);
            return;
        }
        if (m_player.IsDead()) {
            app.State().lastBattlePerfect = false;
            app.ChangeScene(SceneKind::Result, m_stage, false);
        }
    }

    void Render(GameApp&, Renderer& renderer) override {
        if (m_stageController) {
            m_stageController->Draw(renderer, m_player);
        }
        m_player.Draw(renderer);
        DrawPlayerHp(renderer);
        m_gesture.Draw(renderer);
    }

private:
    void DrawPlayerHp(Renderer& renderer) const {
        for (int i = 0; i < m_player.MaxHp(); ++i) {
            renderer.DrawImage(i < m_player.Hp() ? L"Assets\\playerui\\heart1.png" : L"Assets\\playerui\\heart2.png",
                50.0f + static_cast<float>(i) * 58.0f, CanvasHeight - 50.0f, 50.0f, 50.0f);
        }
    }

private:
    int m_stage = 1;
    Player m_player;
    GestureCanvas m_gesture;
    std::unique_ptr<StageController> m_stageController;
};

class ResultScene final : public Scene {
public:
    ResultScene(int stage, bool cleared) : m_stage(stage), m_cleared(cleared) {}

    void OnEnter(GameApp& app) override {
        if (!m_cleared || m_rewardApplied) {
            return;
        }

        GameState& state = app.State();
        const int index = m_stage - 1;
        const bool perfect = state.lastBattlePerfect;
        if (perfect) {
            if (state.stageCoin[index] == 0) {
                state.coin += 2;
            } else if (state.stageCoin[index] == 1) {
                state.coin += 1;
            }
            state.stageCoin[index] = 2;
            state.stageClear[index] = 2;
        } else {
            if (state.stageClear[index] != 2) {
                state.stageClear[index] = 1;
            }
            if (state.stageCoin[index] == 0) {
                state.coin += 1;
                state.stageCoin[index] = 1;
            }
        }
        m_rewardApplied = true;
    }

    void Update(GameApp& app, float) override {
        const InputState& input = app.Input();
        if (Clicked(input, 478.0f, 67.0f, 584.0f, 175.0f)) {
            app.ChangeScene(SceneKind::Home);
        } else if (Clicked(input, 678.0f, 67.0f, 784.0f, 175.0f)) {
            app.ChangeScene(SceneKind::Battle, m_stage);
        }
    }

    void Render(GameApp& app, Renderer& renderer) override {
        renderer.DrawImage(L"Assets\\배경\\black_background.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        if (m_cleared) {
            renderer.DrawImage(L"Assets\\배경\\clear.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
            DrawStageFood(renderer, m_stage);
            if (!app.State().lastBattlePerfect) {
                renderer.DrawImage(L"Assets\\배경\\perfect_no.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
            }
        } else {
            renderer.DrawImage(L"Assets\\배경\\fail.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
            DrawStageFood(renderer, m_stage);
            renderer.DrawImage(L"Assets\\배경\\fail_no.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        }
    }

private:
    int m_stage = 1;
    bool m_cleared = false;
    bool m_rewardApplied = false;
};

class EndingScene final : public Scene {
public:
    void OnEnter(GameApp&) override {
        m_current = 0;
        m_alpha = 0.0f;
    }

    void Update(GameApp& app, float deltaTime) override {
        m_alpha = std::min(1.0f, m_alpha + deltaTime * 0.8f);
        if (app.Input().WasMouseReleased()) {
            if (m_alpha < 1.0f) {
                m_alpha = 1.0f;
            } else if (m_current < 6) {
                ++m_current;
                m_alpha = 0.0f;
            } else {
                app.ChangeScene(SceneKind::Home);
            }
        }
    }

    void Render(GameApp&, Renderer& renderer) override {
        renderer.DrawImage(L"Assets\\배경\\black_background.png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        for (int i = 0; i < m_current; ++i) {
            renderer.DrawImage(L"Assets\\배경\\end_" + std::to_wstring(i) + L".png", CanvasWidth * 0.5f, CanvasHeight * 0.5f);
        }
        renderer.DrawImage(L"Assets\\배경\\end_" + std::to_wstring(m_current) + L".png",
            CanvasWidth * 0.5f, CanvasHeight * 0.5f, -1.0f, -1.0f, m_alpha);
    }

private:
    int m_current = 0;
    float m_alpha = 0.0f;
};
}

std::unique_ptr<Scene> CreateScene(SceneKind kind, int value, bool flag) {
    switch (kind) {
    case SceneKind::Logo:
        return std::make_unique<LogoScene>();
    case SceneKind::Title:
        return std::make_unique<TitleScene>();
    case SceneKind::Home:
        return std::make_unique<HomeScene>();
    case SceneKind::Option:
        return std::make_unique<OptionScene>(static_cast<SceneKind>(value));
    case SceneKind::Shop:
        return std::make_unique<ShopScene>();
    case SceneKind::Confirm:
        return std::make_unique<ConfirmScene>(value);
    case SceneKind::Battle:
        return std::make_unique<BattleScene>(value);
    case SceneKind::Result:
        return std::make_unique<ResultScene>(value, flag);
    case SceneKind::Ending:
        return std::make_unique<EndingScene>();
    default:
        return std::make_unique<TitleScene>();
    }
}
