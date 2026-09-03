#pragma once

#include <Windows.h>
#include <dwrite.h>
#include <d2d1.h>
#include "../DX2DClasses/SceneManager.h"
#include "../DX2DClasses/ColorBrush.h"
#include "../DX2DClasses/Image.h"

class CPlayer;
class CFruitManager;
class CBasket;
class CFruit;

enum class EGameState
{
	TITLE,
	PLAYING,
	RESULT
};

namespace DX2DClasses
{
	class CDriect2DFramwork;
}

class CGameScene : public DX2DClasses::ISceneManager
{
private:
	DX2DClasses::CImage* m_pBackgroundImage;
	bool m_isResultSoundPlayed;
	int TScore;
	int m_nScore;
	int m_nTimeScore;
	int m_nHighScores[3];

	IDWriteTextFormat* m_pScoreTextFormat;
	ID2D1SolidColorBrush* m_pScoreBrush;

	CPlayer* m_pPlayer;
	CBasket* m_pBasket;
	CFruitManager* m_pFruitManager;

	CFruit* m_pTopFruit;

	HWND m_hWnd;
	DX2DClasses::CDriect2DFramwork* m_pDX2DFramework;

	float m_fGameTime;
	float m_fGameLimitTime;

	IDWriteTextFormat* m_pTimerTextFormat;
	ID2D1SolidColorBrush* m_pTimerBrush;

	float m_fGoalY;
	ID2D1SolidColorBrush* m_pGoalBrush;

	bool m_isGameOver;

	ID2D1SolidColorBrush* m_pResultBrush;

	D2D1_RECT_F m_restartButtonRect;
	D2D1_RECT_F m_mainButtonRect;
	D2D1_RECT_F m_startButtonRect;
	D2D1_RECT_F m_exitButtonRect;

private:
	void DrawTimer();

	void CheckGoalCollision();
	void DrawGoal();

	void DrawScore();

	void DrawResult();

	void RestartGame();
	void GoToMainMenu();

	void DrawTitle();

	void AddHighScore(int score);
	void DrawHighScores();

public:
	CGameScene();
	virtual ~CGameScene();

public:
	EGameState m_eGameState;
	void CheckCollision();
	void CheckBasketCollision();
	void CheckFruitStackCollision();
	void StackFruit(CFruit* pFruit);

	void UpdateStackedFruit();

	virtual void Initialize(
		HWND hWnd,
		DX2DClasses::CDriect2DFramwork* pDX2DFramework
	) override;

	void OnMouseClick(int x, int y);

	void ProcessFruitScore(CFruit* pFruit);
	void ProcessBomb();
	void CreateTopFruitCollider();
	void DropFruits(int count);

	virtual void Release() override;

	virtual void Update() override;

	virtual void Draw() override;

	bool m_bForceEnd = false; // ⭐ 릴레이 강제 종료 확인용 플래그
	int GetTotalScore() const { return TScore; } // ⭐ 최종 점수 반환
};
