#pragma once
#include <Windows.h>

namespace SceneManager {

	enum  class SceneType
	{
		Title,
		Ranking,
		Settings,
		NameInput,
		GameA,
		GameB,
		GameC,
		GameD,
		GameE
	};
	void Initialize(HWND hWnd);
	void ChangeScene(SceneType nextScene);


	//각자 씬 관리 부분 포팅 후 다를 경우 통일시키기
	void Update();
	void Draw();
	void Release();

	SceneType GetCurrentScene();
	void OnKeyDown(WPARAM wParam);
	void OnLButtonDown(int mx, int my);
	void OnMouseMove(int mx, int my);
	void OnKeyUp(WPARAM wParam);
};