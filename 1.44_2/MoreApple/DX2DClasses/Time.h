#pragma once
#include <Windows.h>

class CTime
{
private:
	static float m_fPrevTime;

public:
	static float GetDeltaTime()
	{
		return timeGetTime() / 1000.0f;
	}

	static float GetDeltaTimeModify()
	{
		float currentTime = timeGetTime() / 1000.0f;

		float deltaTime = currentTime - m_fPrevTime;

		m_fPrevTime = currentTime;

		return deltaTime;
	}
};