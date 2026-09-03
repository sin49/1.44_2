#include "Time.h"
#pragma comment(lib, "winmm.lib")

float CTime::m_fPrevTime = timeGetTime() / 1000.0f;