#pragma once

// Python resource.py에 흩어져 있던 전역 진행 상태를 한 곳에 모은 구조체다.
struct GameState {
    int coin = 0;

    // 0: 미클리어, 1: 일반 클리어, 2: 완벽 클리어
    int stageClear[3] = { 0, 0, 0 };
    int stageCoin[3] = { 0, 0, 0 };

    int bgmVolume = 4;
    int effectVolume = 4;

    int selectedWeapon = 1;
    bool weaponOwned[4] = { true, false, false, false };

    int playerMaxHp = 3;
    int playerAttack = 20;

    bool endingSeen = false;
    bool lastBattlePerfect = false;

    bool AllStagesCleared() const {
        return stageClear[0] > 0 && stageClear[1] > 0 && stageClear[2] > 0;
    }
};

