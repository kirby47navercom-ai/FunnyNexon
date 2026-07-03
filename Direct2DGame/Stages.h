#pragma once

#include "GestureRecognizer.h"
#include "Patterns.h"

#include <memory>

class Player;
class Renderer;

struct StageOutcome {
    bool cleared = false;
};

class StageController {
public:
    virtual ~StageController() = default;
    virtual void Reset() = 0;
    virtual void Update(float deltaTime, Player& player) = 0;
    virtual void ApplyGesture(PatternId id, Player& player) = 0;
    virtual void Draw(Renderer& renderer, const Player& player) const = 0;
    virtual bool IsCleared() const = 0;
};

std::unique_ptr<StageController> CreateStageController(int stage);
std::unique_ptr<StageController> CreateStage1Controller();
std::unique_ptr<StageController> CreateStage2Controller();
std::unique_ptr<StageController> CreateStage3Controller();
