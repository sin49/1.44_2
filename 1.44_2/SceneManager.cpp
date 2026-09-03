#include "SceneManager.h"
#include "ScoreManager.h"
#include<Windows.h>
#include <d2d1.h> // 해상도 리사이즈용
// 1. 각 게임의 껍데기(네임스페이스) 미리 선언
namespace TitleScene { void Init(); void Update(); void Draw(); void Release();  void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my);
}
namespace SettingsScene { void Init(); void Update(); void Draw(); void Release(); void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my);
}
namespace RankingScene { void Init(); void Update(); void Draw(); void Release();  void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); }
namespace GameA {
    void Init(HWND hWnd); void Update(); void Draw(); void Release();
    bool IsForceEnd(); void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore();
}
namespace GameB { // 오토체스용 빈 껍데기
    void Init(HWND hWnd); void Update(); void Draw(); void Release();
    void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore();
}
namespace GameC { // 로봇 점프용 빈 껍데기
    void Init(HWND hWnd); void Update(); void Draw(); void Release();
    bool IsForceEnd(); void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore();
}
namespace GameD { 
    void Init(HWND hWnd); void Update(); void Draw(); void Release(); void InputKeyUp(WPARAM wParam);
    void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore();
}
namespace NameInputScene { void Init(); void Update(); void Draw(); void Release(); void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); }

HWND g_hWnd = nullptr;
SceneManager::SceneType g_currentScene = SceneManager::SceneType::Title;
DWORD g_lastSceneChangeTime = 0; // ⭐ 씬 전환 딜레이용 변수

extern ID2D1HwndRenderTarget* g_pRenderTarget; // main.cpp의 렌더타겟 가져오기
namespace SceneManager
{
    void ResizeWindowForScene(int width, int height) {
        if (!g_hWnd) return;
        RECT rc = { 0, 0, width, height };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE); // 창 테두리 픽셀 보정
        SetWindowPos(g_hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

        // Direct2D 도화지 사이즈도 윈도우에 맞춰 동기화
        if (g_pRenderTarget) {
            g_pRenderTarget->Resize(D2D1::SizeU(width, height));
        }
    }
    void Initialize(HWND hWnd) {
        g_hWnd = hWnd;
        ChangeScene(SceneType::Title);
    }

    void ChangeScene(SceneType nextScene) {
        DWORD now = GetTickCount();
        if (now - g_lastSceneChangeTime < 400) return;
        g_lastSceneChangeTime = now;

        // 기존 씬 해제
        switch (g_currentScene) {
        case SceneType::Title:
            TitleScene::Release();
            break;
        case SceneType::Settings:
            SettingsScene::Release();
            break;
        case SceneType::GameA:
            GameA::Release();
            break;
        case SceneType::GameB:
            GameB::Release();
            break;
        case SceneType::GameC:
            GameC::Release();
            break;
        case SceneType::GameD:
            GameD::Release();
            break;
        case SceneType::NameInput:
            NameInputScene::Release();
            break;
        case SceneType::Ranking:
            RankingScene::Release();
            break;
        }
        g_currentScene = nextScene;
        switch (g_currentScene) {
        case SceneType::GameA: ResizeWindowForScene(800, 450); break;  // INVEMA
        case SceneType::GameB: ResizeWindowForScene(1280, 720); break; // AutoBattle
        case SceneType::GameC: ResizeWindowForScene(1024, 768); break; // RobotJump
        case SceneType::GameD: ResizeWindowForScene(800, 450); break;  // BadukShooting
        default: ResizeWindowForScene(1280, 720); break; // Title, Ranking, Settings, NameInput
        }
        // 새 씬 초기화
        switch (g_currentScene) {
        case SceneType::Title:
            TitleScene::Init();
            break;
        case SceneType::Settings:
            SettingsScene::Init();
            break;
        case SceneType::GameA:
            GameA::Init(g_hWnd);
            break;
        case SceneType::GameB:
            GameB::Init(g_hWnd);
            break;
        case SceneType::GameC:
            GameC::Init(g_hWnd);
            break;
        case SceneType::GameD:
            GameD::Init(g_hWnd);
            break;
        case SceneType::NameInput:
            NameInputScene::Init();
            break;
        case SceneType::Ranking:
            RankingScene::Init();
            break;
        }

    }

    void Update() {
        // 현재 씬 업데이트
        switch (g_currentScene) {
        case SceneType::Title:
            TitleScene::Update();
            break;
        case SceneType::Settings:
            SettingsScene::Update();
            break;
        case SceneType::GameA:
            GameA::Update();
            break;
        case SceneType::GameB:
            GameB::Update();
            break;
        case SceneType::GameC:
            GameC::Update();
            break;
        case SceneType::GameD:
            GameD::Update();
            break;
        case SceneType::NameInput:
            NameInputScene::Update();
            break;
        case SceneType::Ranking:
            RankingScene::Update();
            break;
        }


        // ==========================================
        // ⭐ 릴레이 (바통 터치) 로직 감시
        // ==========================================
        if (g_currentScene == SceneType::GameA) {
            if (GameA::IsForceEnd()) {
                // END RELAY 버튼을 누른 경우 -> 바로 이름 입력으로!
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameA, GameA::GetScore());
                ChangeScene(SceneType::NameInput);
            }
            else if (GameA::IsGameOver()) {
                // 게임오버 후 정상 진행 -> Game C로
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameA, GameA::GetScore());
                ChangeScene(SceneType::GameC);
            }
        }
        else if (g_currentScene == SceneType::GameC) {
            if (GameC::IsForceEnd()) {
                // END RELAY 버튼을 누른 경우 -> 바로 이름 입력으로!
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameC, GameC::GetScore());
                ChangeScene(SceneType::NameInput);
            }
            else if (GameC::IsGameOver()) {
                // 게임오버 후 정상 진행 -> Game D로
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameC, GameC::GetScore());
                ChangeScene(SceneType::GameD);
            }
        }
        else if (g_currentScene == SceneType::GameD && GameD::IsGameOver()) {
            // 마지막 게임은 어떻게 죽든 무조건 이름 입력으로!
            ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameD, GameD::GetScore());
            ChangeScene(SceneType::NameInput);
        }
        else if (g_currentScene == SceneType::GameB && GameB::IsGameOver()) {
            ChangeScene(SceneType::Title); // 스탠드얼론은 타이틀로
        }
    }
    void Draw() {
        switch (g_currentScene) {
        case SceneType::Title:
            TitleScene::Draw();
            break;
        case SceneType::Settings:
            SettingsScene::Draw();
            break;
        case SceneType::Ranking:
            RankingScene::Draw();
            break;
        case SceneType::NameInput:
            NameInputScene::Draw();
            break;
        case SceneType::GameA:
            GameA::Draw();
            break;
        case SceneType::GameB:
            GameB::Draw();
            break;
        case SceneType::GameC:
            GameC::Draw();
            break;
        case SceneType::GameD:
            GameD::Draw();
            break;
        }
    }

    void Release() {
        switch (g_currentScene) {
        case SceneType::Title:
            TitleScene::Release();
            break;
        case SceneType::Settings:
            SettingsScene::Release();
            break;
        case SceneType::GameA:
            GameA::Release();
            break;
        case SceneType::GameB:
            GameB::Release();
            break;
        case SceneType::GameC:
            GameC::Release();
            break;
        case SceneType::GameD:
            GameD::Release();
            break;
        case SceneType::NameInput:
            NameInputScene::Release();
            break;
        case SceneType::Ranking:
            RankingScene::Release();
            break;
        }
    }

    SceneType GetCurrentScene() { return g_currentScene; }


    // 키/마우스 입력 분배
   // ==========================================
// ⭐ 메인 프레임워크 키/마우스 입력 라우팅 
// ==========================================
    namespace SceneManager {
        void OnKeyDown(WPARAM wParam) {
            if (g_currentScene == SceneType::Title) TitleScene::InputKey(wParam);
            else if (g_currentScene == SceneType::Settings) SettingsScene::InputKey(wParam);
            else if (g_currentScene == SceneType::Ranking) RankingScene::InputKey(wParam);
            else if (g_currentScene == SceneType::NameInput) NameInputScene::InputKey(wParam);
            else if (g_currentScene == SceneType::GameA) GameA::InputKey(wParam);
            else if (g_currentScene == SceneType::GameB) GameB::InputKey(wParam);
            else if (g_currentScene == SceneType::GameC) GameC::InputKey(wParam);
            else if (g_currentScene == SceneType::GameD) GameD::InputKey(wParam);
        }
        void OnKeyUp(WPARAM wParam) {
            if (g_currentScene == SceneType::GameD) GameD::InputKeyUp(wParam);
        }
        void OnLButtonDown(int mx, int my) {
            if (g_currentScene == SceneType::Title) TitleScene::InputMouseClick(mx, my);
            else if (g_currentScene == SceneType::Settings) SettingsScene::InputMouseClick(mx, my);
            else if (g_currentScene == SceneType::Ranking) RankingScene::InputMouseClick(mx, my);
            else if (g_currentScene == SceneType::NameInput) NameInputScene::InputMouseClick(mx, my);
            else if (g_currentScene == SceneType::GameA) GameA::InputMouseClick(mx, my);
            else if (g_currentScene == SceneType::GameB) GameB::InputMouseClick(mx, my);
            else if (g_currentScene == SceneType::GameC) GameC::InputMouseClick(mx, my);
            else if (g_currentScene == SceneType::GameD) GameD::InputMouseClick(mx, my);
        }
        void OnMouseMove(int mx, int my) {
            if (g_currentScene == SceneType::Title) TitleScene::InputMouseMove(mx, my);
            else if (g_currentScene == SceneType::Settings) SettingsScene::InputMouseMove(mx, my);
            else if (g_currentScene == SceneType::GameA) GameA::InputMouseMove(mx, my);
            else if (g_currentScene == SceneType::GameB) GameB::InputMouseMove(mx, my);
            else if (g_currentScene == SceneType::GameC) GameC::InputMouseMove(mx, my);
        }
    }
}

namespace SceneManager {
    void OnKeyDown(WPARAM wParam) {
        if (g_currentScene == SceneType::Title) TitleScene::InputKey(wParam);
        else if (g_currentScene == SceneType::Settings) SettingsScene::InputKey(wParam);
        else if (g_currentScene == SceneType::Ranking) RankingScene::InputKey(wParam);
        else if (g_currentScene == SceneType::NameInput) NameInputScene::InputKey(wParam);
        else if (g_currentScene == SceneType::GameA) GameA::InputKey(wParam);
        else if (g_currentScene == SceneType::GameB) GameB::InputKey(wParam);
        else if (g_currentScene == SceneType::GameC) GameC::InputKey(wParam);
        else if (g_currentScene == SceneType::GameD) GameD::InputKey(wParam);
    }

    void OnKeyUp(WPARAM wParam) {
        if (g_currentScene == SceneType::GameD) GameD::InputKeyUp(wParam);
    }

    void OnLButtonDown(int mx, int my) {
        if (g_currentScene == SceneType::Title) TitleScene::InputMouseClick(mx, my);
        else if (g_currentScene == SceneType::Settings) SettingsScene::InputMouseClick(mx, my);
        else if (g_currentScene == SceneType::Ranking) RankingScene::InputMouseClick(mx, my);
        else if (g_currentScene == SceneType::NameInput) NameInputScene::InputMouseClick(mx, my);
        else if (g_currentScene == SceneType::GameA) GameA::InputMouseClick(mx, my);
        else if (g_currentScene == SceneType::GameB) GameB::InputMouseClick(mx, my);
        else if (g_currentScene == SceneType::GameC) GameC::InputMouseClick(mx, my);
        else if (g_currentScene == SceneType::GameD) GameD::InputMouseClick(mx, my);
    }

    void OnMouseMove(int mx, int my) {
        if (g_currentScene == SceneType::Title) TitleScene::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::Settings) SettingsScene::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::GameA) GameA::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::GameB) GameB::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::GameC) GameC::InputMouseMove(mx, my);
    }
}








