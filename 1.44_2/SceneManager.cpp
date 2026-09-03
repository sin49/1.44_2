#include "SceneManager.h"
#include "ScoreManager.h"
#include <Windows.h>
#include "soundManager.h"
#include <d2d1.h> // 해상도 리사이즈용

// 1. 각 게임의 껍데기(네임스페이스) 미리 선언
namespace TitleScene { void Init(); void Update(); void Draw(); void Release();  void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); }
namespace SettingsScene { void Init(); void Update(); void Draw(); void Release(); void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); }
namespace RankingScene { void Init(); void Update(); void Draw(); void Release();  void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); }
namespace NameInputScene { void Init(); void Update(); void Draw(); void Release(); void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); }

namespace GameA {
    void Init(HWND hWnd); void Update(); void Draw(); void Release();
    bool IsForceEnd(); void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore();
}
namespace GameB {
    void Init(HWND hWnd); void Update(); void Draw(); void Release();
    void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore();
}
namespace GameC {
    void Init(HWND hWnd); void Update(); void Draw(); void Release();
    bool IsForceEnd(); void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore();
}
namespace GameD {
    void Init(HWND hWnd); void Update(); void Draw(); void Release(); void InputKeyUp(WPARAM wParam);
    void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore();
}
namespace GameE { // ⭐ 사과게임 껍데기 추가
    void Init(HWND hWnd); void Update(); void Draw(); void Release();
    void InputKey(WPARAM wParam); void InputMouseClick(int mx, int my); void InputMouseMove(int mx, int my); bool IsGameOver(); int GetScore(); bool IsForceEnd();
}

HWND g_hWnd = nullptr;
SceneManager::SceneType g_currentScene = SceneManager::SceneType::Title;
DWORD g_lastSceneChangeTime = 0; // 씬 전환 딜레이용 변수

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
        if (now - g_lastSceneChangeTime < 400) return; // 0.4초 딜레이
        g_lastSceneChangeTime = now;

        while (ShowCursor(TRUE) < 0);
        // 기존 씬 해제
        switch (g_currentScene) {
        case SceneType::Title: TitleScene::Release(); break;
        case SceneType::Settings: SettingsScene::Release(); break;
        case SceneType::Ranking: RankingScene::Release(); break;
        case SceneType::NameInput: NameInputScene::Release(); break;
        case SceneType::GameA: GameA::Release(); break;
        case SceneType::GameB: GameB::Release(); break;
        case SceneType::GameC: GameC::Release(); break;
        case SceneType::GameD: GameD::Release(); break;
        case SceneType::GameE: GameE::Release(); break;
        }

        g_currentScene = nextScene;

        // 새 씬 해상도 변경
        switch (g_currentScene) {
        case SceneType::GameA: ResizeWindowForScene(800, 450); break;  // INVEMA
        case SceneType::GameB: ResizeWindowForScene(1280, 720); break; // AutoBattle
        case SceneType::GameC: ResizeWindowForScene(1024, 768); break; // RobotJump
        case SceneType::GameD: ResizeWindowForScene(800, 450); break;  // BadukShooting
        case SceneType::GameE: ResizeWindowForScene(600, 800); break;  // ⭐ MoreApple
        default: ResizeWindowForScene(1280, 720); break; // 프레임워크 기본 해상도
        }

        // 새 씬 초기화
        switch (g_currentScene) {
        case SceneType::Title: TitleScene::Init(); break;
        case SceneType::Settings: SettingsScene::Init(); break;
        case SceneType::Ranking: RankingScene::Init(); break;
        case SceneType::NameInput: NameInputScene::Init(); break;
        case SceneType::GameA: GameA::Init(g_hWnd); break;
        case SceneType::GameB: GameB::Init(g_hWnd); break;
        case SceneType::GameC: GameC::Init(g_hWnd); break;
        case SceneType::GameD: GameD::Init(g_hWnd); break;
        case SceneType::GameE: GameE::Init(g_hWnd); break;
        }
    }
   
    void Update() {
        // 현재 씬 업데이트
        switch (g_currentScene) {
        case SceneType::Title: TitleScene::Update(); break;
        case SceneType::Settings: SettingsScene::Update(); break;
        case SceneType::Ranking: RankingScene::Update(); break;
        case SceneType::NameInput: NameInputScene::Update(); break;
        case SceneType::GameA: GameA::Update(); break;
        case SceneType::GameB: GameB::Update(); break;
        case SceneType::GameC: GameC::Update(); break;
        case SceneType::GameD: GameD::Update(); break;
        case SceneType::GameE: GameE::Update(); break;
        }

        // ==========================================
        // ⭐ 릴레이 (바통 터치) 로직 감시
        // ==========================================
        if (g_currentScene == SceneType::GameA) {
            if (GameA::IsForceEnd()) {
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameA, GameA::GetScore());
                ChangeScene(SceneType::NameInput);
            }
            else if (GameA::IsGameOver()) {
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameA, GameA::GetScore());
                ChangeScene(SceneType::GameC);
            }
        }
        else if (g_currentScene == SceneType::GameC) {
            if (GameC::IsForceEnd()) {
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameC, GameC::GetScore());
                ChangeScene(SceneType::NameInput);
            }
            else if (GameC::IsGameOver()) {
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameC, GameC::GetScore());
                ChangeScene(SceneType::GameD);
            }
        }
        else if (g_currentScene == SceneType::GameD && GameD::IsGameOver()) {
            ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameD, GameD::GetScore());
            // 알까기 끝나면 사과게임(GameE)으로 이동!
            ChangeScene(SceneType::GameE);
        }
        else if (g_currentScene == SceneType::GameE) {
            if (GameE::IsForceEnd()) {
                // ⭐ 릴레이 탈출 시 점수 저장
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameE, GameE::GetScore());
                ChangeScene(SceneType::NameInput);
            }
            else if (GameE::IsGameOver()) {
                // ⭐ 정상 종료 시 점수 저장 (이 줄이 빠져있었습니다 ㅠㅠ)
                ScoreManager::RecordCurrentGameScore(ScoreManager::GameType::GameE, GameE::GetScore());
                ChangeScene(SceneType::NameInput);
            }
        }
        else if (g_currentScene == SceneType::GameB && GameB::IsGameOver()) {
            ChangeScene(SceneType::Title); // 오토배틀 스탠드얼론은 끝나면 타이틀로 복귀
        }
    }

    void Draw() {
        switch (g_currentScene) {
        case SceneType::Title: TitleScene::Draw(); break;
        case SceneType::Settings: SettingsScene::Draw(); break;
        case SceneType::Ranking: RankingScene::Draw(); break;
        case SceneType::NameInput: NameInputScene::Draw(); break;
        case SceneType::GameA: GameA::Draw(); break;
        case SceneType::GameB: GameB::Draw(); break;
        case SceneType::GameC: GameC::Draw(); break;
        case SceneType::GameD: GameD::Draw(); break;
        case SceneType::GameE: GameE::Draw(); break;
        }
    }

    void Release() {
        switch (g_currentScene) {
        case SceneType::Title: TitleScene::Release(); break;
        case SceneType::Settings: SettingsScene::Release(); break;
        case SceneType::Ranking: RankingScene::Release(); break;
        case SceneType::NameInput: NameInputScene::Release(); break;
        case SceneType::GameA: GameA::Release(); break;
        case SceneType::GameB: GameB::Release(); break;
        case SceneType::GameC: GameC::Release(); break;
        case SceneType::GameD: GameD::Release(); break;
        case SceneType::GameE: GameE::Release(); break;
        }
    }

    SceneType GetCurrentScene() { return g_currentScene; }

    // ==========================================
    // ⭐ 메인 프레임워크 키/마우스 입력 라우팅 
    // ==========================================
    void OnKeyDown(WPARAM wParam) {
        if (g_currentScene == SceneType::Title) TitleScene::InputKey(wParam);
        else if (g_currentScene == SceneType::Settings) SettingsScene::InputKey(wParam);
        else if (g_currentScene == SceneType::Ranking) RankingScene::InputKey(wParam);
        else if (g_currentScene == SceneType::NameInput) NameInputScene::InputKey(wParam);
        else if (g_currentScene == SceneType::GameA) GameA::InputKey(wParam);
        else if (g_currentScene == SceneType::GameB) GameB::InputKey(wParam);
        else if (g_currentScene == SceneType::GameC) GameC::InputKey(wParam);
        else if (g_currentScene == SceneType::GameD) GameD::InputKey(wParam);
        else if (g_currentScene == SceneType::GameE) GameE::InputKey(wParam);
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
        else if (g_currentScene == SceneType::GameE) GameE::InputMouseClick(mx, my);
    }

    void OnMouseMove(int mx, int my) {
        if (g_currentScene == SceneType::Title) TitleScene::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::Settings) SettingsScene::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::GameA) GameA::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::GameB) GameB::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::GameC) GameC::InputMouseMove(mx, my);
        else if (g_currentScene == SceneType::GameE) GameE::InputMouseMove(mx, my);
    }
}