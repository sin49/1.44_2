#include <Windows.h>//윈도우 창 관련 헤더
#include <d2d1.h> // 1. Direct2D 기본 헤더 추가
#include "DrawManager.h"
#include "soundManager.h"
#include "SceneManager.h"
#include "ScoreManager.h"
#pragma comment(lib, "d2d1.lib")

using namespace D2D1;
using namespace DrawManager;

namespace GameA {
	void Init(HWND hwnd);
	void Update();
	void Draw();
	void Release();
	void InputKey(WPARAM wParam);
	void InputMouseClick(int mx, int my);
	void InputMouseMove(int mx, int my);
}

ID2D1Factory* g_pD2DFactory = nullptr;
ID2D1HwndRenderTarget* g_pRenderTarget = nullptr;






LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM LParam);

int APIENTRY wWinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, LPWSTR lpcmdline, int ncmdshow)
{

	WNDCLASS wc = { 0 };
	// 초기화
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hinstance;
	wc.lpszClassName = L"DX2DFRAMEWORK";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	//초기화 끝난 wc를 쓰라고 운영체제에게 전달
	RegisterClass(&wc);

	//창 생성
	HWND hwnd = CreateWindow(
		L"DX2DFramework",
		L"1.44MB MULTI-GAME",
		WS_OVERLAPPEDWINDOW,
		100, 100,
		1280, 720,
		NULL, NULL, hinstance, NULL);

	//창 실제로 띄우기
	ShowWindow(hwnd, ncmdshow);

	//메시지 루프
	MSG msg;
	//계속 메시지 받을 때까지 계속 돌려....
	while (GetMessage(&msg, nullptr, 0, 0)) {//창이꺼지기 전까지란 뜻?
		TranslateMessage(&msg);//키보드입력
		DispatchMessage(&msg);//메시지를 windowproc에 던지는 역활
	}
	//창이 닫힘=종료
	return 0;
}

void Initialize(HWND hWnd)
{
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);

	if (g_pD2DFactory)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);

		g_pD2DFactory->CreateHwndRenderTarget(
			RenderTargetProperties(),
			HwndRenderTargetProperties(
				hWnd,
				SizeU(rc.right - rc.left, rc.bottom - rc.top)
			),
			&g_pRenderTarget
		);

		DrawManager::InitializeDraw(g_pRenderTarget);



		// 1. 사운드 시스템 켜기
		SoundManager::Initialize();

		// 2. 씬 매니저 초기화
		SceneManager::Initialize(hWnd);

		ScoreManager::Initialize();
		
		GameA::Init(hWnd);

		// 3. 60프레임 타이머 가동
		SetTimer(hWnd, 1, 16, NULL); // 대략 60프레임 (16ms)
	}
}
#pragma region Update
void Update() 
{
	SceneManager::Update();

}

#pragma endregion

#pragma region Draw


void Draw() {
	// 1. 레트로 서바(GameA)는 GDI 방식이므로 Direct2D 밖에서 단독 호출!
	if (SceneManager::GetCurrentScene() == SceneManager::SceneType::GameA ||
		SceneManager::GetCurrentScene() == SceneManager::SceneType::GameB || // ⭐ GameB 추가!
		SceneManager::GetCurrentScene() == SceneManager::SceneType::GameC) {
		SceneManager::Draw();
	}
	// 2. 타이틀, 셋팅 등 프레임워크 씬들은 Direct2D 방식으로 호출!
	else {
		if (g_pRenderTarget != nullptr) {
			g_pRenderTarget->BeginDraw();
			g_pRenderTarget->Clear(ColorF(ColorF::Black));

			SceneManager::Draw(); // Direct2D 기반 그리기

			g_pRenderTarget->EndDraw();
		}
	}
}
#pragma endregion

#pragma region Release
void Release() {
	SceneManager::Release();
	SoundManager::Release();
	GameA::Release();

	if (g_pRenderTarget) { g_pRenderTarget->Release(); g_pRenderTarget = nullptr; }
	if (g_pD2DFactory) { g_pD2DFactory->Release(); g_pD2DFactory = nullptr; }
}
#pragma endregion

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CREATE:
		Initialize(hwnd);
		break;
	case WM_DESTROY:
		Release();
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if ((lParam & (1 << 30)) == 0) {
			SceneManager::OnKeyDown(wParam);
		}
		break;
	case WM_KEYUP:

		SceneManager::OnKeyUp(wParam);
		break;
	case WM_LBUTTONDOWN:
		SceneManager::OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
		break;
	
	case WM_MOUSEMOVE:
		SceneManager::OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_TIMER:
		Update();
		InvalidateRect(hwnd, NULL, false);
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		Draw();
		EndPaint(hwnd, &ps);
	}
	break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}


// ==========================================
// ⭐ 네임 인풋 씬 (해상도 중앙 정렬 및 꾹 누름 방지)
// ==========================================
namespace NameInputScene {
	char initials[4] = "___"; int currentIndex = 0;
	void Init() { strcpy_s(initials, "___"); currentIndex = 0; }
	void Update() {} void Release() {}
	void Draw() {
		DrawManager::FillRect(0, 0, 1280, 720, D2D1::ColorF::Black);
		// 1280x720 해상도 정중앙 배치
		DrawManager::DrawWhiteText(440, 250, 500, 50, L"ENTER YOUR INITIALS", 40.0f, D2D1::ColorF::DeepPink);
		wchar_t buf[16]; swprintf_s(buf, 16, L"%C   %C   %C", initials[0], initials[1], initials[2]);
		DrawManager::DrawWhiteText(510, 350, 300, 60, buf, 60.0f, D2D1::ColorF::White);
		if (currentIndex == 3) DrawManager::DrawWhiteText(450, 500, 400, 50, L"PRESS [SPACE] TO SAVE", 30.0f, D2D1::ColorF::LimeGreen);
	}
	void InputKey(WPARAM wParam) {
		if (wParam >= 'A' && wParam <= 'Z' && currentIndex < 3) { initials[currentIndex] = (char)wParam; currentIndex++; }
		else if (wParam == VK_BACK && currentIndex > 0) { currentIndex--; initials[currentIndex] = '_'; }
		else if (wParam == VK_SPACE && currentIndex == 3) { // 엔터 대신 스페이스
			ScoreManager::FinalizeRelayAndSave(initials);
			SceneManager::ChangeScene(SceneManager::SceneType::Ranking);
		}
	}
	void InputMouseClick(int mx, int my) {} void InputMouseMove(int mx, int my) {}
}

// ==========================================
// ⭐ 랭킹 씬 (마우스 복귀 지원)
// ==========================================
namespace RankingScene {
	void Init() {} void Update() {} void Release() {}
	void Draw() {
		DrawManager::FillRect(0, 0, 1280, 720, D2D1::ColorF(0.05f, 0.05f, 0.1f));
		DrawManager::DrawWhiteText(450, 50, 400, 40, L" TOTAL RANKING ", 35.0f, D2D1::ColorF::Gold);
		for (int i = 0; i < 3; i++) {
			auto entry = ScoreManager::GetScore(ScoreManager::GameType::Total, i);
			wchar_t buf[64]; swprintf_s(buf, 64, L"%d. %S - %d", i + 1, entry.initial, entry.score);
			DrawManager::DrawWhiteText(530, 110 + i * 40, 300, 30, buf, 26.0f, D2D1::ColorF::White);
		}

		auto DrawSubRank = [](float x, float y, const wchar_t* title, ScoreManager::GameType type) {
			DrawManager::DrawWhiteText(x, y, 300, 30, title, 20.0f, D2D1::ColorF::Cyan);
			for (int i = 0; i < 3; i++) {
				auto entry = ScoreManager::GetScore(type, i);
				wchar_t buf[64]; swprintf_s(buf, 64, L"%d. %S : %d", i + 1, entry.initial, entry.score);
				DrawManager::DrawWhiteText(x, y + 40 + i * 30, 300, 30, buf, 18.0f, D2D1::ColorF::LightGray);
			}
		};
		DrawSubRank(200, 350, L"[ GAME A: SURVIVAL ]", ScoreManager::GameType::GameA);
		DrawSubRank(550, 350, L"[ GAME C: JUMP ]", ScoreManager::GameType::GameC);
		DrawSubRank(900, 350, L"[ GAME D: SHOOTER ]", ScoreManager::GameType::GameD);

		// 복귀 버튼
		DrawManager::FillRect(440, 630, 400, 50, D2D1::ColorF(0.2f, 0.2f, 0.3f));
		DrawManager::DrawWhiteText(470, 640, 400, 40, L"[CLICK] or [SPACE] TO TITLE", 22.0f, D2D1::ColorF::Yellow);
	}
	void InputKey(WPARAM wParam) { if (wParam == VK_SPACE) SceneManager::ChangeScene(SceneManager::SceneType::Title); }
	void InputMouseClick(int mx, int my) { if (my >= 630 && my <= 680) SceneManager::ChangeScene(SceneManager::SceneType::Title); }
	void InputMouseMove(int mx, int my) {}
}