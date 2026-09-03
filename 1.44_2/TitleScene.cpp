#include "SceneManager.h"
#include "DrawManager.h"
#include "SoundManager.h"
#include <windows.h>

using namespace DrawManager;

namespace TitleScene
{
    int selectedBtn = 0;
    bool isKeyLocked = false;
    int hoverBtn = -1; // 마우스 호버 감지
    const int BTN_COUNT = 5;

    void Init() { selectedBtn = 0; hoverBtn = -1; }

    // 방향키 이동만 Update에서 처리
    void Update() {
        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            if (!isKeyLocked) { selectedBtn = (selectedBtn - 1 + BTN_COUNT) % BTN_COUNT; SoundManager::PlayScratch(); isKeyLocked = true; }
        }
        else if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
            if (!isKeyLocked) { selectedBtn = (selectedBtn + 1) % BTN_COUNT; SoundManager::PlayScratch(); isKeyLocked = true; }
        }
        else { isKeyLocked = false; }
    }

    void Draw() {
        FillRect(0.0f, 0.0f, 1280.0f, 720.0f, D2D1::ColorF(0.05f, 0.05f, 0.1f));
        FillRect(340.0f, 80.0f, 600.0f, 100.0f, D2D1::ColorF(0.1f, 0.1f, 0.2f));

        DrawWhiteText(360.0f, 110.0f, 600.0f, 80.0f, L"1.44MB MULTI-ARCADE", 40.0f, D2D1::ColorF::Cyan);

        float startY = 240.0f; float btnWidth = 400.0f; float btnHeight = 55.0f; float startX = 440.0f;
        const wchar_t* btnLabels[5] = { L"1. RELAY MODE (A->C->D)", L"2. AUTO BATTLE", L"3. RANKINGS", L"4. SETTINGS", L"5. EXIT" };

        for (int i = 0; i < BTN_COUNT; ++i) {
            float y = startY + i * 75.0f;
            if (selectedBtn == i || hoverBtn == i) FillRect(startX, y, btnWidth, btnHeight, D2D1::ColorF::DeepPink);
            else FillRect(startX, y, btnWidth, btnHeight, D2D1::ColorF(0.2f, 0.2f, 0.3f));

            DrawWhiteText(startX + 30.0f, y + 12.0f, btnWidth, btnHeight, btnLabels[i], 22.0f, D2D1::ColorF::White);
        }
        DrawWhiteText(450.0f, 650.0f, 500.0f, 30.0f, L"[UP/DOWN] NAVIGATE   |   [SPACE/CLICK] SELECT", 18.0f, D2D1::ColorF::Yellow);
    }

    void ExecuteCommand(int idx) {
        SoundManager::PlayCoin();
        if (idx == 0) SceneManager::ChangeScene(SceneManager::SceneType::GameA);
        else if (idx == 1) SceneManager::ChangeScene(SceneManager::SceneType::GameB);
        else if (idx == 2) SceneManager::ChangeScene(SceneManager::SceneType::Ranking);
        else if (idx == 3) SceneManager::ChangeScene(SceneManager::SceneType::Settings);
        else if (idx == 4) PostQuitMessage(0);
    }

    // ⭐ 스페이스 꾹 누름을 방지하기 위해 단발성 이벤트인 InputKey에서 처리!
    void InputKey(WPARAM wParam) {
        if (wParam == VK_SPACE) ExecuteCommand(selectedBtn);
    }

    void InputMouseMove(int mx, int my) {
        hoverBtn = -1;
        if (mx >= 440 && mx <= 840) {
            for (int i = 0; i < BTN_COUNT; ++i) {
                float y = 240.0f + i * 75.0f;
                if (my >= y && my <= y + 55.0f) { hoverBtn = i; selectedBtn = i; }
            }
        }
    }

    void InputMouseClick(int mx, int my) {
        if (hoverBtn != -1) ExecuteCommand(hoverBtn);
    }
    void Release() {}
}