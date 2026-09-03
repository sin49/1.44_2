#pragma once
#include <d2d1.h>
#include <dwrite.h>

namespace DrawManager
{
    void InitializeDraw(ID2D1HwndRenderTarget* pRenderTarget);
    void ReleaseDraw();

    void FillRect(float x, float y, float w, float h, D2D1::ColorF color);
     void DrawWhiteText(float x, float y, float w, float h, const wchar_t* text, float fontSize = 20.0f, D2D1::ColorF color = D2D1::ColorF::White);
};