#include "GameScene.h"
#include "Player.h"
#include "FruitManager.h"
#include "Basket.h"
#include "SoundManager.h"

#include "../DX2DClasses/Driect2DFramework.h"
#include "../DX2DClasses/Colliders.h"
#include "../DX2DClasses/DebugHelper.h"

#include <assert.h>
#include <cmath>
#include <algorithm>

using namespace DX2DClasses;

CGameScene::CGameScene()
	: m_pPlayer(nullptr)
	, m_pBackgroundImage(nullptr)
	, m_hWnd(nullptr)
	, m_pDX2DFramework(nullptr)
	, m_pFruitManager(nullptr)
	, m_pBasket(nullptr)
	, m_pTopFruit(nullptr)
	, m_fGameTime(0.0f)
	, m_fGameLimitTime(60.0f)
	, m_pTimerTextFormat(nullptr)
	, m_pTimerBrush(nullptr)
	, m_fGoalY(300.0f)
	, m_pGoalBrush(nullptr)
	, m_isGameOver(false)
	, m_nScore(0)
	, TScore(0)
	, m_nTimeScore(0)
	, m_pScoreTextFormat(nullptr)
	, m_pScoreBrush(nullptr)
	, m_eGameState(EGameState::TITLE)
	, m_pResultBrush(nullptr)
	, m_isResultSoundPlayed(false)
{
	m_nHighScores[0] = 0;
	m_nHighScores[1] = 0;
	m_nHighScores[2] = 0;
}

CGameScene::~CGameScene()
{
	Release();
}

void CGameScene::Initialize(
	HWND hWnd,
	CDriect2DFramwork* pDX2DFramework)
{
	m_hWnd = hWnd;
	m_pDX2DFramework = pDX2DFramework;

	m_pBackgroundImage = new CImage(
		m_pDX2DFramework->GetD2DRenderTarget(),
		m_pDX2DFramework->GetImagingFactory(),
		1
	);
	m_pBackgroundImage->ManualLoadImage(
		m_hWnd,
		L"Images\\Background.png"
	);

	// SoundManager 초기화
	SoundManager::Initialize();

	m_restartButtonRect =
		D2D1::RectF(
			300.0f,
			450.0f,
			500.0f,
			510.0f
		);

	m_mainButtonRect =
		D2D1::RectF(
			300.0f,
			550.0f,
			500.0f,
			610.0f
		);

	m_startButtonRect =
		D2D1::RectF(
			200.0f,
			450.0f,
			400.0f,
			510.0f
		);

	m_exitButtonRect = D2D1::RectF(
		200.0f,
		550.0f,
		400.0f,
		610.0f
	);

	HRESULT hr;

	hr = m_pDX2DFramework->GetWriteFactory()->CreateTextFormat(
		L"Arial",
		nullptr,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"",
		&m_pTimerTextFormat
	);

	assert(hr == S_OK);

	hr = m_pDX2DFramework->GetD2DRenderTarget()->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::White),
		&m_pTimerBrush
	);

	assert(hr == S_OK);

	hr = m_pDX2DFramework->GetD2DRenderTarget()->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Red),
		&m_pGoalBrush
	);

	assert(hr == S_OK);

	hr = m_pDX2DFramework->GetWriteFactory()->CreateTextFormat(
		L"Arial",
		nullptr,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"",
		&m_pScoreTextFormat
	);

	assert(hr == S_OK);

	hr = m_pDX2DFramework->GetD2DRenderTarget()->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::White),
		&m_pScoreBrush
	);

	assert(hr == S_OK);

	hr = m_pDX2DFramework->GetD2DRenderTarget()->CreateSolidColorBrush(
		D2D1::ColorF(
			0.0f,
			0.0f,
			0.0f,
			0.8f
		),
		&m_pResultBrush
	);

	assert(hr == S_OK);

	// -------------------------
	// Player 생성
	// -------------------------
	m_pPlayer = new CPlayer();

	// -------------------------
	// Player 이미지 생성
	// -------------------------
	CImage* pIdleImage = new CImage(
		m_pDX2DFramework->GetD2DRenderTarget(),
		m_pDX2DFramework->GetImagingFactory(),
		4
	);

	// -------------------------
	// 이미지 로드
	// -------------------------
	pIdleImage->ManualLoadImage(
		m_hWnd,
		L"Images\\Player(32x32)\\player_idle%02d.png"
	);

	CImage* pRunImage = new CImage(
		m_pDX2DFramework->GetD2DRenderTarget(),
		m_pDX2DFramework->GetImagingFactory(),
		8
	);

	pRunImage->ManualLoadImage(
		m_hWnd,
		L"Images\\Player(32x32)\\player_run%02d.png"
	);

	m_pPlayer->SetIdleImage(pIdleImage);
	m_pPlayer->SetRunImage(pRunImage);

	// -------------------------
	// CGameObject 초기화
	// -------------------------
	m_pPlayer->Initialize(
		pIdleImage,
		true,
		true
	);

	// -------------------------
	// Player 위치 설정
	// -------------------------
	m_pPlayer->GetTransform().SetTRS(
		SVector2(400.0f, 900.0f),
		0.0f,
		SVector2(1.0f, 1.0f)
	);

	// Animator 초기 프레임
	m_pPlayer->GetAnimator()->SetFrame(0);

	// FruitManager 초기화
	m_pFruitManager = new CFruitManager();

	m_pFruitManager->Initialize(
		hWnd,
		pDX2DFramework
	);

	CImage* pBasketImage = new CImage(
		m_pDX2DFramework->GetD2DRenderTarget(),
		m_pDX2DFramework->GetImagingFactory(),
		1
	);

	pBasketImage->ManualLoadImage(
		m_hWnd,
		L"Images\\Items\\basket.png"
	);

	m_pBasket = new CBasket();

	m_pBasket->Initialize(
		pBasketImage,
		false,
		true
	);

	m_pBasket->SetOffset(
		SVector2(0.0f, -15.0f)
	);

	m_pBasket->InitializeCollider();

	SoundManager::PlayBGM(L"title.mid");
}

void CGameScene::Release()
{
	SoundManager::Release();
	SoundManager::StopBGM();

	if (m_pBackgroundImage)
	{
		delete m_pBackgroundImage;
		m_pBackgroundImage = nullptr;
	}

	if (m_pResultBrush)
	{
		m_pResultBrush->Release();
		m_pResultBrush = nullptr;
	}

	if (m_pScoreTextFormat)
	{
		m_pScoreTextFormat->Release();
		m_pScoreTextFormat = nullptr;
	}

	if (m_pScoreBrush)
	{
		m_pScoreBrush->Release();
		m_pScoreBrush = nullptr;
	}

	if (m_pTimerBrush)
	{
		m_pTimerBrush->Release();
		m_pTimerBrush = nullptr;
	}

	if (m_pTimerTextFormat)
	{
		m_pTimerTextFormat->Release();
		m_pTimerTextFormat = nullptr;
	}

	if (m_pTimerBrush)
	{
		m_pTimerBrush->Release();
		m_pTimerBrush = nullptr;
	}

	if (m_pPlayer)
	{
		delete m_pPlayer;
		m_pPlayer = nullptr;
	}

	if (m_pBasket)
	{
		delete m_pBasket;
		m_pBasket = nullptr;
	}

	if (m_pFruitManager)
	{
		delete m_pFruitManager;
		m_pFruitManager = nullptr;
	}
}

void CGameScene::Update()
{
	if (m_eGameState == EGameState::TITLE)
	{
		return;
	}

	if (m_eGameState == EGameState::RESULT)
	{
		return;
	}

	const float deltaTime = 1.0f / 60.0f;

	if (m_fGameTime < m_fGameLimitTime)
	{
		m_fGameTime += deltaTime;
	}

	if (m_fGameTime > m_fGameLimitTime)
	{
		m_fGameTime = m_fGameLimitTime;

		m_nTimeScore = 0;
	
		AddHighScore(m_nScore);
		TScore = m_nScore;
		m_eGameState = EGameState::RESULT;

		if (!m_isResultSoundPlayed)
		{
			SoundManager::PlayFanfare_2();
			m_isResultSoundPlayed = true;
		}

		return;
	}

	if (m_pPlayer)
	{
		m_pPlayer->Update();

		if (m_pBasket)
		{
			m_pBasket->UpdatePosition(
				m_pPlayer->GetTransform().GetTransrate(),
				m_pPlayer->IsLeft()
			);

			m_pBasket->Update();
		}
	}

	if (m_pFruitManager)
		m_pFruitManager->Update();

	CheckCollision();

	UpdateStackedFruit();
}

void CGameScene::Draw()
{
	if (m_pBackgroundImage)
	{
		m_pBackgroundImage->DrawBitmap(
			SVector2(0.0f, 0.0f),
			SVector2(4.13f, 3.06f),
			0.0f,
			0
		);
	}

	if (m_eGameState == EGameState::TITLE)
	{
		DrawTitle();
	}
	else if (m_eGameState == EGameState::PLAYING)
	{
		if (m_pPlayer)
			m_pPlayer->Draw();

		if (m_pBasket)
			m_pBasket->Draw();

		if (m_pFruitManager)
			m_pFruitManager->Draw();

		DrawGoal();
		DrawTimer();
		DrawScore();
	}
	else if (m_eGameState == EGameState::RESULT)
	{
		DrawResult();
	}
}

void CGameScene::CheckCollision()
{
	CheckBasketCollision();
	CheckFruitStackCollision();
	CheckGoalCollision();
}

void CGameScene::CheckBasketCollision()
{
	// 이미 과일이 쌓여 있다면
	// 바구니 콜라이더는 더 이상 사용하지 않는다.
	if (m_pTopFruit != nullptr)
		return;

	if (m_pBasket == nullptr)
		return;

	if (m_pBasket->GetCollider() == nullptr)
		return;

	CRectCollider* pBasketCollider =
		static_cast<CRectCollider*>(m_pBasket->GetCollider());

	for (CFruit* pFruit : m_pFruitManager->GetFruits())
	{
		if (pFruit == nullptr)
			continue;

		// 이미 쌓인 과일은 검사하지 않는다.
		if (pFruit->IsStacked())
			continue;

		if (pFruit->GetCollider() == nullptr)
			continue;

		CCircleCollider* pFruitCollider =
			static_cast<CCircleCollider*>(pFruit->GetCollider());

		if (pFruitCollider->ToRect(pBasketCollider))
		{
			if (pFruit->GetFruitType() == EFruitType::BOMB)
			{
				ProcessBomb();
			}
			else
			{
				StackFruit(pFruit);
			}
			return;
		}
	}
}

void CGameScene::CheckFruitStackCollision()
{
	if (m_pTopFruit == nullptr)
		return;

	if (m_pTopFruit->GetCollider() == nullptr)
		return;

	CCircleCollider* pTopCollider =
		static_cast<CCircleCollider*>(
			m_pTopFruit->GetCollider()
			);

	for (CFruit* pFruit : m_pFruitManager->GetFruits())
	{
		if (pFruit == nullptr)
			continue;

		// 이미 쌓여 있는 과일
		if (pFruit->IsStacked())
			continue;

		if (pFruit->GetCollider() == nullptr)
			continue;

		CCircleCollider* pFruitCollider =
			static_cast<CCircleCollider*>(
				pFruit->GetCollider()
				);

		if (pFruitCollider->ToCircle(pTopCollider))
		{
			StackFruit(pFruit);
			return;
		}
	}
}

void CGameScene::StackFruit(CFruit* pFruit)
{
	if (pFruit == nullptr)
		return;

	if (pFruit->GetFruitType() == EFruitType::BOMB)
	{
		ProcessFruitScore(pFruit);

		pFruit->SetStacked(false);

		SoundManager::PlayExplosion();

		return;
	}

	CCircleCollider* pFruitCollider =
		static_cast<CCircleCollider*>(
			pFruit->GetCollider()
			);

	if (pFruitCollider == nullptr)
		return;

	// 처음 쌓이는 과일
	if (m_pTopFruit == nullptr)
	{
		SVector2 basketPos =
			m_pBasket->GetTransform().GetTransrate();


		CRectCollider* pBasketCollider =
			static_cast<CRectCollider*>(
				m_pBasket->GetCollider()
				);

		if (pBasketCollider == nullptr)
			return;

		// 바구니의 월드 좌표
		SVector2 basketTL =
			pBasketCollider->GetWorldTL();

		SVector2 basketBR =
			pBasketCollider->GetWorldBR();

		float basketCenterX =
			(basketTL.x + basketBR.x) * 0.5f;

		float basketTopY =
			basketTL.y;

		float fruitRadius =
			pFruitCollider->GetRadius();

		// 과일을 바구니 위에 배치
		SVector2 pos =
			pFruit->GetTransform().GetTransrate();

		if (pFruit->GetFruitType() == EFruitType::APPLE)
		{
			pos.x = basketCenterX - 18.0f;
			pos.y = basketTopY - fruitRadius - 3.0f;
		}
		else
		{
			pos.x = basketCenterX - 8.0f;
			pos.y = basketTopY - fruitRadius;
		}

		pFruit->GetTransform().SetTransrate(pos);

		// 바구니와 체리 사이의 상대 위치 저장
	
		pFruit->SetStackOffset(
			pos - basketPos
		);

		pFruit->SetStacked(true);

		// 더 이상 떨어지지 않음
		pFruit->SetStacked(true);

		// 바구니 콜라이더 제거
		delete m_pBasket->GetCollider();
		m_pBasket->SetCollider(nullptr);

		// 이 과일을 최상단 과일로 지정
		m_pTopFruit = pFruit;

		SoundManager::PlayCoin();

		ProcessFruitScore(pFruit);

		return;
	}

	// 두 번째 과일부터
	CCircleCollider* pTopCollider =
		static_cast<CCircleCollider*>(
			m_pTopFruit->GetCollider()
			);

	if (pTopCollider == nullptr)
		return;

	SVector2 topPos =
		pTopCollider->GetWorldPos();

	float topRadius =
		pTopCollider->GetRadius();

	float fruitRadius =
		pFruitCollider->GetRadius();

	// 기존 최상단 과일 위에 배치
	SVector2 pos =
		pFruit->GetTransform().GetTransrate();

	if (pFruit->GetFruitType() == EFruitType::APPLE)
	{
		pos.x = topPos.x - 18.0f;
		pos.y = topPos.y - topRadius - fruitRadius - 10.0f;
	}
	else
	{
		pos.x = topPos.x - 11.0f;
		pos.y = topPos.y - topRadius - fruitRadius - 3.0f;
	}

	pFruit->GetTransform().SetTransrate(pos);

	// 바구니 기준 상대 위치 저장
	SVector2 basketPos =
		m_pBasket->GetTransform().GetTransrate();

	pFruit->SetStackOffset(
		pos - basketPos
	);

	// 더 이상 떨어지지 않음
	pFruit->SetStacked(true);

	// 기존 최상단 과일의 콜라이더 제거
	delete m_pTopFruit->GetCollider();
	m_pTopFruit->SetCollider(nullptr);

	// 새로운 과일이 최상단 과일
	m_pTopFruit = pFruit;

	SoundManager::PlayCoin();

	ProcessFruitScore(pFruit);
}

void CGameScene::UpdateStackedFruit()
{
	if (m_pBasket == nullptr)
		return;

	SVector2 basketPos =
		m_pBasket->GetTransform().GetTransrate();

	for (CFruit* pFruit : m_pFruitManager->GetFruits())
	{
		if (pFruit == nullptr)
			continue;

		// 쌓이지 않은 체리는 건드리지 않는다.
		if (!pFruit->IsStacked())
			continue;

		SVector2 offset =
			pFruit->GetStackOffset();

		SVector2 newPos;

		newPos.x = basketPos.x + offset.x;
		newPos.y = basketPos.y + offset.y;

		pFruit->GetTransform().SetTransrate(newPos);
	}
}

void CGameScene::DrawTimer()
{
	if (m_pTimerTextFormat == nullptr)
		return;

	if (m_pTimerBrush == nullptr)
		return;

	m_pDX2DFramework->GetD2DRenderTarget()->SetTransform(
		D2D1::Matrix3x2F::Identity()
	);

	int remainTime =
		static_cast<int>(
			ceil(m_fGameLimitTime - m_fGameTime)
			);

	if (remainTime < 0)
		remainTime = 0;

	wchar_t timerText[32];

	swprintf_s(
		timerText,
		L"Time : %d",
		remainTime
	);

	D2D1_RECT_F textRect =
		D2D1::RectF(
			20.0f,
			20.0f,
			250.0f,
			60.0f
		);

	m_pDX2DFramework->GetD2DRenderTarget()->DrawText(
		timerText,
		static_cast<UINT32>(wcslen(timerText)),
		m_pTimerTextFormat,
		textRect,
		m_pTimerBrush
	);
}

void CGameScene::DrawGoal()
{
	if (m_pGoalBrush == nullptr)
		return;

	ID2D1HwndRenderTarget* pRenderTarget =
		m_pDX2DFramework->GetD2DRenderTarget();

	// UI/선 그리기 전에 Transform 초기화
	pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Identity()
	);

	D2D1_POINT_2F start =
		D2D1::Point2F(0.0f, m_fGoalY);

	D2D1_POINT_2F end =
		D2D1::Point2F(800.0f, m_fGoalY);

	pRenderTarget->DrawLine(
		start,
		end,
		m_pGoalBrush,
		3.0f
	);
}

void CGameScene::CheckGoalCollision()
{
	if (m_eGameState == EGameState::RESULT)
		return;

	if (m_pTopFruit == nullptr)
		return;

	if (m_pTopFruit->GetCollider() == nullptr)
		return;

	CCircleCollider* pCollider =
		static_cast<CCircleCollider*>(
			m_pTopFruit->GetCollider()
			);

	SVector2 center =
		pCollider->GetWorldPos();

	float radius =
		pCollider->GetRadius();

	float fruitTop =
		center.y - radius;

	if (fruitTop <= m_fGoalY)
	{
		m_nTimeScore =
			static_cast<int>(
				(m_fGameLimitTime - m_fGameTime) * 200
				);

		int totalScore = m_nScore + m_nTimeScore;

		AddHighScore(totalScore);

		m_eGameState = EGameState::RESULT;

		if (!m_isResultSoundPlayed)
		{
			SoundManager::PlayFanfare_2();
			m_isResultSoundPlayed = true;
		}

		CDebugHelper::LogConsole(
			"GAME OVER : Goal Reached"
		);
	}
}

void CGameScene::DrawScore()
{
	if (m_pScoreTextFormat == nullptr)
		return;

	if (m_pScoreBrush == nullptr)
		return;

	ID2D1HwndRenderTarget* pRenderTarget =
		m_pDX2DFramework->GetD2DRenderTarget();

	// 과일이 사용한 Transform 초기화
	pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Identity()
	);

	wchar_t scoreText[32];

	swprintf_s(
		scoreText,
		L"Score : %d",
		m_nScore
	);

	D2D1_RECT_F textRect =
		D2D1::RectF(
			650.0f,
			800.0f,
			800.0f,
			0.0f
		);

	pRenderTarget->DrawText(
		scoreText,
		static_cast<UINT32>(wcslen(scoreText)),
		m_pScoreTextFormat,
		textRect,
		m_pScoreBrush
	);
}

void CGameScene::DrawResult()
{
	ID2D1HwndRenderTarget* pRenderTarget =
		m_pDX2DFramework->GetD2DRenderTarget();

	pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Identity()
	);

	// 결과창 배경
	D2D1_RECT_F resultRect =
		D2D1::RectF(
			200.0f,
			150.0f,
			600.0f,
			650.0f
		);

	pRenderTarget->FillRectangle(
		resultRect,
		m_pResultBrush
	);

	// RESULT
	wchar_t resultText[] = L"RESULT";

	D2D1_RECT_F resultTextRect =
		D2D1::RectF(
			250.0f,
			220.0f,
			550.0f,
			280.0f
		);

	pRenderTarget->DrawText(
		resultText,
		static_cast<UINT32>(wcslen(resultText)),
		m_pScoreTextFormat,
		resultTextRect,
		m_pScoreBrush
	);

	wchar_t scoreText[64];

	swprintf_s(
		scoreText,
		L"Score : %d",
		m_nScore
	);

	D2D1_RECT_F scoreRect =
		D2D1::RectF(
			250.0f,
			300.0f,
			550.0f,
			350.0f
		);

	pRenderTarget->DrawText(
		scoreText,
		static_cast<UINT32>(wcslen(scoreText)),
		m_pScoreTextFormat,
		scoreRect,
		m_pScoreBrush
	);

	// --------------------------------
	// Time Score
	// --------------------------------
	wchar_t timeScoreText[64];

	swprintf_s(
		timeScoreText,
		L"Time Score : %d",
		m_nTimeScore
	);

	D2D1_RECT_F timeScoreRect =
		D2D1::RectF(
			250.0f,
			350.0f,
			550.0f,
			400.0f
		);

	pRenderTarget->DrawText(
		timeScoreText,
		static_cast<UINT32>(wcslen(timeScoreText)),
		m_pScoreTextFormat,
		timeScoreRect,
		m_pScoreBrush
	);

	// --------------------------------
	// Total Score
	// --------------------------------
	wchar_t totalScoreText[64];

	int totalScore = m_nScore + m_nTimeScore;

	swprintf_s(
		totalScoreText,
		L"Total Score : %d",
		totalScore
	);

	D2D1_RECT_F totalScoreRect =
		D2D1::RectF(
			250.0f,
			400.0f,
			550.0f,
			450.0f
		);

	pRenderTarget->DrawText(
		totalScoreText,
		static_cast<UINT32>(wcslen(totalScoreText)),
		m_pScoreTextFormat,
		totalScoreRect,
		m_pScoreBrush
	);

	// 다시하기 버튼
	pRenderTarget->DrawRectangle(
		m_restartButtonRect,
		m_pScoreBrush,
		2.0f
	);

	// 메인화면 버튼
	pRenderTarget->DrawRectangle(
		m_mainButtonRect,
		m_pScoreBrush,
		2.0f
	);

	wchar_t restartText[] = L"Restart";
	wchar_t mainText[] = L"Main Menu";

	pRenderTarget->DrawText(
		restartText,
		static_cast<UINT32>(wcslen(restartText)),
		m_pScoreTextFormat,
		m_restartButtonRect,
		m_pScoreBrush
	);

	pRenderTarget->DrawText(
		mainText,
		static_cast<UINT32>(wcslen(mainText)),
		m_pScoreTextFormat,
		m_mainButtonRect,
		m_pScoreBrush
	);
}

void CGameScene::OnMouseClick(int x, int y)
{
	if (m_eGameState == EGameState::TITLE)
	{
		if (x >= m_startButtonRect.left &&
			x <= m_startButtonRect.right &&
			y >= m_startButtonRect.top &&
			y <= m_startButtonRect.bottom)
		{
			SoundManager::PlayFanfare_0();

			m_eGameState = EGameState::PLAYING;
		}

		// EXIT 버튼
		if (x >= m_exitButtonRect.left &&
			x <= m_exitButtonRect.right &&
			y >= m_exitButtonRect.top &&
			y <= m_exitButtonRect.bottom)
		{
			m_bForceEnd = true;
			m_eGameState == EGameState::RESULT;
			return;
		}

		return;
	}

	if (m_eGameState != EGameState::RESULT)
	{
		CDebugHelper::LogConsole("Not Result State");
		return;
	}

	if (x >= m_restartButtonRect.left &&
		x <= m_restartButtonRect.right &&
		y >= m_restartButtonRect.top &&
		y <= m_restartButtonRect.bottom)
	{
		CDebugHelper::LogConsole("Restart Button Click");
		SoundManager::PlayFanfare_0();

		RestartGame();
		return;
	}

	if (x >= m_mainButtonRect.left &&
		x <= m_mainButtonRect.right &&
		y >= m_mainButtonRect.top &&
		y <= m_mainButtonRect.bottom)
	{
		CDebugHelper::LogConsole("Main Menu Button Click");

		GoToMainMenu();
		return;
	}
}

void CGameScene::RestartGame()
{
	m_fGameTime = 0.0f;
	m_nScore = 0;
	m_nTimeScore = 0;

	m_isResultSoundPlayed = false;

	m_eGameState = EGameState::PLAYING;

	m_pTopFruit = nullptr;

	if (m_pFruitManager)
		m_pFruitManager->ClearFruits();

	if (m_pBasket)
	{
		m_pBasket->InitializeCollider();

		m_pBasket->GetTransform().SetTransrate(
			0.0f,
			0.0f
		);
	}

	if (m_pPlayer)
	{
		m_pPlayer->GetTransform().SetTRS(
			SVector2(400.0f, 900.0f),
			0.0f,
			SVector2(1.0f, 1.0f)
		);

		m_pPlayer->GetAnimator()->SetFrame(0);
	}
}

void CGameScene::GoToMainMenu()
{
	m_fGameTime = 0.0f;
	m_nScore = 0;
	m_nTimeScore = 0;

	m_isResultSoundPlayed = false;

	m_pTopFruit = nullptr;

	if (m_pFruitManager)
		m_pFruitManager->ClearFruits();

	if (m_pPlayer)
	{
		m_pPlayer->GetTransform().SetTRS(
			SVector2(400.0f, 900.0f),
			0.0f,
			SVector2(1.0f, 1.0f)
		);

		m_pPlayer->GetAnimator()->SetFrame(0);
	}

	if (m_pBasket)
	{
		m_pBasket->GetTransform().SetTransrate(
			0.0f,
			0.0f
		);

		// 바구니 콜라이더가 제거된 상태라면 다시 생성
		if (m_pBasket->GetCollider() == nullptr)
		{
			m_pBasket->InitializeCollider();
		}
	}

	m_eGameState = EGameState::TITLE;

	SoundManager::PlayBGM(L"title.mid");
}

void CGameScene::DrawTitle()
{
	ID2D1HwndRenderTarget* pRenderTarget =
		m_pDX2DFramework->GetD2DRenderTarget();

	pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Identity()
	);

	D2D1_RECT_F titleBackgroundRect =
		D2D1::RectF(
			100.0f,
			150.0f,
			500.0f,
			650.0f
		);

	pRenderTarget->FillRectangle(
		titleBackgroundRect,
		m_pResultBrush
	);

	// 제목
	wchar_t titleText[] = L"MORE APPLE";

	D2D1_RECT_F titleRect =
		D2D1::RectF(
			110.0f,
			200.0f,
			510.0f,
			280.0f
		);

	pRenderTarget->DrawText(
		titleText,
		static_cast<UINT32>(wcslen(titleText)),
		m_pScoreTextFormat,
		titleRect,
		m_pScoreBrush
	);

	//DrawHighScores();

	wchar_t guideText[] = L"MOVE: Left / Right Arrow |  [CLICK] INTERACT";
	D2D1_RECT_F guideRect = D2D1::RectF(120.0f, 320.0f, 500.0f, 360.0f);
	pRenderTarget->DrawTextW(
		guideText,
		static_cast<UINT32>(wcslen(guideText)),
		m_pScoreTextFormat,
		guideRect,
		m_pScoreBrush
	);

	pRenderTarget->DrawRectangle(
		m_startButtonRect,
		m_pScoreBrush,
		2.0f
	);

	wchar_t startText[] = L"START";

	pRenderTarget->DrawText(
		startText,
		static_cast<UINT32>(wcslen(startText)),
		m_pScoreTextFormat,
		m_startButtonRect,
		m_pScoreBrush
	);

	pRenderTarget->DrawRectangle(
		m_exitButtonRect,
		m_pScoreBrush,
		2.0f
	);

	wchar_t exitText[] = L"EXIT";

	pRenderTarget->DrawText(
		exitText,
		static_cast<UINT32>(wcslen(exitText)),
		m_pScoreTextFormat,
		m_exitButtonRect,
		m_pScoreBrush
	);
}

void CGameScene::AddHighScore(int score)
{
	TScore = score;
	// 3등보다 낮으면 등록하지 않음
	if (score <= m_nHighScores[2])
		return;

	// 새로운 점수를 일단 3등 자리에 넣음
	m_nHighScores[2] = score;

	// 내림차순 정렬
	if (m_nHighScores[2] > m_nHighScores[1])
	{
		int temp = m_nHighScores[2];
		m_nHighScores[2] = m_nHighScores[1];
		m_nHighScores[1] = temp;
	}

	if (m_nHighScores[1] > m_nHighScores[0])
	{
		int temp = m_nHighScores[1];
		m_nHighScores[1] = m_nHighScores[0];
		m_nHighScores[0] = temp;
	}


}

void CGameScene::DrawHighScores()
{
	ID2D1HwndRenderTarget* pRenderTarget =
		m_pDX2DFramework->GetD2DRenderTarget();

	pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Identity()
	);

	wchar_t scoreText[64];

	for (int i = 0; i < 3; i++)
	{
		swprintf_s(
			scoreText,
			L"TOP %d   %d",
			i + 1,
			m_nHighScores[i]
		);

		D2D1_RECT_F scoreRect =
			D2D1::RectF(
				150.0f,
				270.0f + i * 50.0f,
				450.0f,
				310.0f + i * 50.0f
			);

		pRenderTarget->DrawText(
			scoreText,
			static_cast<UINT32>(wcslen(scoreText)),
			m_pScoreTextFormat,
			scoreRect,
			m_pScoreBrush
		);
	}
}

void CGameScene::ProcessFruitScore(CFruit* pFruit)
{
	if (pFruit == nullptr)
		return;

	switch (pFruit->GetFruitType())
	{
	case EFruitType::CHERRY:
		m_nScore += 100;
		break;

	case EFruitType::APPLE:
		m_nScore += 200;
		break;

	case EFruitType::BOMB:
		m_nScore -= 300;

		if (m_nScore < 0)
			m_nScore = 0;

		ProcessBomb();
		m_pFruitManager->RemoveBomb(pFruit);
		break;
	}
}

void CGameScene::DropFruits(int count)
{
	int droppedCount = 0;

	for (CFruit* pFruit : m_pFruitManager->GetFruits())
	{
		if (pFruit == nullptr)
			continue;

		if (!pFruit->IsStacked())
			continue;

		pFruit->SetStacked(false);

		droppedCount++;

		if (droppedCount >= count)
			break;
	}

	m_pTopFruit = nullptr;

	if (m_pBasket && m_pBasket->GetCollider() == nullptr)
	{
		m_pBasket->InitializeCollider();
	}
}

void CGameScene::ProcessBomb()
{
	if (m_pFruitManager == nullptr)
		return;

	CFruit* pNewTopFruit = m_pFruitManager->RemoveStackedFruits(3);

	m_pTopFruit = pNewTopFruit;

	// 삭제 후 과일이 남아 있다면
	if (m_pTopFruit != nullptr)
	{
		// 새로운 최상단 과일에 Collider 생성
		m_pFruitManager->CreateCollider(m_pTopFruit);
	}
	else
	{
		// 과일이 하나도 없다면 바구니 Collider 복구
		if (m_pBasket != nullptr &&
			m_pBasket->GetCollider() == nullptr)
		{
			m_pBasket->InitializeCollider();
		}
	}
}

void CGameScene::CreateTopFruitCollider()
{
	if (m_pTopFruit == nullptr)
		return;

	// 이미 Collider가 있다면 생성하지 않는다.
	if (m_pTopFruit->GetCollider() != nullptr)
		return;

	CCircleCollider* pCollider =
		new CCircleCollider(
			m_pTopFruit->GetTransformPtr()
		);

	pCollider->InitCollider(
		m_pTopFruit->GetTransformPtr(),
		SVector2(11.0f, 11.0f),
		m_pFruitManager->GetCherryImageSize(),
		0.3f
	);

	m_pTopFruit->SetCollider(pCollider);
}

