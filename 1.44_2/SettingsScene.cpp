#include "SceneManager.h"
#include "DrawManager.h"
#include "SoundManager.h"
#include <windows.h>

using namespace DrawManager;

namespace SettingsScene
{
    int selectedSettingIdx = 0;
    float masterVol = 1.0f, bgmVol = 1.0f, sfxVol = 1.0f;
    bool isKeyLocked = false;
    int hoverBtn = -1;

    void Init() { selectedSettingIdx = 0; hoverBtn = -1; }

    void Update() {
        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            if (!isKeyLocked) { selectedSettingIdx = (selectedSettingIdx - 1 + 4) % 4; SoundManager::PlayScratch(); isKeyLocked = true; }
        }
        else if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
            if (!isKeyLocked) { selectedSettingIdx = (selectedSettingIdx + 1) % 4; SoundManager::PlayScratch(); isKeyLocked = true; }
        }
        else { isKeyLocked = false; }

        if (selectedSettingIdx == 0) {
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) { masterVol = max(0.0f, masterVol - 0.05f); SoundManager::SetMasterVolume(masterVol); }
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { masterVol = min(1.0f, masterVol + 0.05f); SoundManager::SetMasterVolume(masterVol); }
        }
        else if (selectedSettingIdx == 1) {
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) { bgmVol = max(0.0f, bgmVol - 0.05f); SoundManager::SetBGMVolume(bgmVol); }
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { bgmVol = min(1.0f, bgmVol + 0.05f); SoundManager::SetBGMVolume(bgmVol); }
        }
        else if (selectedSettingIdx == 2) {
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) { sfxVol = max(0.0f, sfxVol - 0.05f); SoundManager::SetSFXVolume(sfxVol); SoundManager::PlayCoin(); }
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { sfxVol = min(1.0f, sfxVol + 0.05f); SoundManager::SetSFXVolume(sfxVol); SoundManager::PlayCoin(); }
        }
    }

    void Draw() {
        FillRect(0.0f, 0.0f, 1280.0f, 720.0f, D2D1::ColorF(0.05f, 0.05f, 0.1f));
        FillRect(340.0f, 80.0f, 600.0f, 100.0f, D2D1::ColorF(0.1f, 0.1f, 0.2f));

        DrawWhiteText(540.0f, 110.0f, 500.0f, 50.0f, L"SETTINGS", 40.0f, D2D1::ColorF::DeepPink);

        float startY = 240.0f;
        auto DrawVolSlider = [&](int idx, float y, const wchar_t* label, float vol) {
            if (selectedSettingIdx == idx || hoverBtn == idx) FillRect(340.0f, y, 600.0f, 55.0f, D2D1::ColorF::ForestGreen);
            else FillRect(340.0f, y, 600.0f, 55.0f, D2D1::ColorF(0.2f, 0.2f, 0.3f));

            DrawWhiteText(370.0f, y + 15.0f, 250.0f, 40.0f, label, 20.0f, D2D1::ColorF::White);
            FillRect(600.0f, y + 20.0f, 300.0f, 15.0f, D2D1::ColorF::Black);
            FillRect(600.0f, y + 20.0f, 300.0f * vol, 15.0f, D2D1::ColorF::Yellow);
        };

        DrawVolSlider(0, startY, L"Master Vol", masterVol);
        DrawVolSlider(1, startY + 80.0f, L"BGM Vol", bgmVol);
        DrawVolSlider(2, startY + 160.0f, L"SFX Vol", sfxVol);

        if (selectedSettingIdx == 3 || hoverBtn == 3) FillRect(340.0f, startY + 240.0f, 600.0f, 55.0f, D2D1::ColorF::DeepPink);
        else FillRect(340.0f, startY + 240.0f, 600.0f, 55.0f, D2D1::ColorF(0.2f, 0.2f, 0.3f));
        DrawWhiteText(590.0f, startY + 255.0f, 250.0f, 40.0f, L"BACK", 22.0f, D2D1::ColorF::White);

        DrawWhiteText(350.0f, 650.0f, 600.0f, 30.0f, L"[UP/DOWN] NAVIGATE | [LEFT/RIGHT] ADJUST | [SPACE] SELECT", 16.0f, D2D1::ColorF::Yellow);
    }

    void ReturnToTitle() { SoundManager::PlayCoin(); SceneManager::ChangeScene(SceneManager::SceneType::Title); }

    void InputKey(WPARAM wParam) { if (wParam == VK_SPACE && selectedSettingIdx == 3) ReturnToTitle(); }

    void InputMouseMove(int mx, int my) {
        hoverBtn = -1;
        if (mx >= 340 && mx <= 940) {
            for (int i = 0; i < 4; ++i) {
                float y = 240.0f + i * 80.0f;
                if (my >= y && my <= y + 55.0f) { hoverBtn = i; selectedSettingIdx = i; }
            }
        }
    }

    void InputMouseClick(int mx, int my) {
        if (hoverBtn == 0) { masterVol = min(1.0f, masterVol + 0.1f); if (masterVol > 1.0f) masterVol = 0.0f; SoundManager::SetMasterVolume(masterVol); }
        else if (hoverBtn == 1) { bgmVol = min(1.0f, bgmVol + 0.1f); if (bgmVol > 1.0f) bgmVol = 0.0f; SoundManager::SetBGMVolume(bgmVol); }
        else if (hoverBtn == 2) { sfxVol = min(1.0f, sfxVol + 0.1f); if (sfxVol > 1.0f) sfxVol = 0.0f; SoundManager::SetSFXVolume(sfxVol); SoundManager::PlayCoin(); }
        else if (hoverBtn == 3) ReturnToTitle();
    }
    void Release() {}
}