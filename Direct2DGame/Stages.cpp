#include "Stages.h"

std::unique_ptr<StageController> CreateStageController(int stage) {
    if (stage == 1) {
        return CreateStage1Controller();
    }
    if (stage == 2) {
        return CreateStage2Controller();
    }
    return CreateStage3Controller();
}
