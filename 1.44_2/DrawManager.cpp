#include "DrawManager.h"
#include <dwrite.h>

#pragma comment(lib, "dwrite.lib")

namespace {
    ID2D1HwndRenderTarget* g_pRenderTargetRef = nullptr;
    IDWriteFactory* g_pDWriteFactory = nullptr;
    IDWriteTextFormat* g_pTextFormat = nullptr;
}

namespace DrawManager
{
    void InitializeDraw(ID2D1HwndRenderTarget* pRenderTarget) {
        g_pRenderTargetRef = pRenderTarget;

        // 1. DirectWrite 공장(Factory) 생성
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&g_pDWriteFactory));

        // 2. 기본 폰트 설정 (맑은 고딕, 크기 20, 한국어 설정)
        if (g_pDWriteFactory) {
            g_pDWriteFactory->CreateTextFormat(
                L"Malgun Gothic",
                NULL,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                20.0f,
                L"ko-KR",
                &g_pTextFormat
            );
        }
    }

    void ReleaseDraw() {
        if (g_pTextFormat) { g_pTextFormat->Release(); g_pTextFormat = nullptr; }
        if (g_pDWriteFactory) { g_pDWriteFactory->Release(); g_pDWriteFactory = nullptr; }
    }

    void FillRect(float x, float y, float w, float h, D2D1::ColorF color) {
        if (!g_pRenderTargetRef) return;
        ID2D1SolidColorBrush* pBrush = nullptr;
        g_pRenderTargetRef->CreateSolidColorBrush(color, &pBrush);
        if (pBrush) {
            D2D1_RECT_F rect = D2D1::RectF(x, y, x + w, y + h);
            g_pRenderTargetRef->FillRectangle(rect, pBrush);
            pBrush->Release();
        }
    }

    // 텍스트 렌더링 함수 구현
    void DrawWhiteText(float x, float y, float w, float h, const wchar_t* text, float fontSize, D2D1::ColorF color) {
        if (!g_pRenderTargetRef || !g_pTextFormat) return;

        ID2D1SolidColorBrush* pBrush = nullptr;
        g_pRenderTargetRef->CreateSolidColorBrush(color, &pBrush);
        if (pBrush) {
            D2D1_RECT_F layoutRect = D2D1::RectF(x, y, x + w, y + h);

            // 폰트 사이즈가 기본과 다를 경우 일시적으로 적용하거나 기본 포맷 사용
            // 여기서는 간단하게 전달받은 텍스트를 출력
            g_pRenderTargetRef->DrawTextW(
                text,
                (UINT32)wcslen(text),
                g_pTextFormat,
                layoutRect,
                pBrush
            );

            pBrush->Release();
        }
    }
}