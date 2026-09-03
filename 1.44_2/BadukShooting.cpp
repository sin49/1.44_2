#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <functional>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

extern ID2D1HwndRenderTarget* g_pRenderTarget;

enum GameState {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER
};

enum EnemyType {
    TYPE_BLACK_STONE,
    TYPE_JANGGI_MA,
    TYPE_JANGGI_KING
};
namespace GameD {
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 450;

    const int BOARD_WIDTH = 600;
    const int BOARD_HEIGHT = 300;
    const float BOARD_START_X = (WINDOW_WIDTH - BOARD_WIDTH) / 2.0f;
    const float BOARD_START_Y = 0.0f;

    const int NUM_ROWS = 13;
    const float CELL_SIZE_PX = (float)BOARD_HEIGHT / (NUM_ROWS - 1);
    const int NUM_COLS = (int)(BOARD_WIDTH / CELL_SIZE_PX) + 1;

    const float PI = 3.14159265f;
    const float GAME_TIME_LIMIT = 60.0f;

    const D2D1_RECT_F BTN_START_RECT = D2D1::RectF(250, 320, 550, 360);
    const D2D1_RECT_F BTN_EXIT_RECT = D2D1::RectF(250, 375, 550, 415);
    const D2D1_RECT_F BTN_RESTART_RECT = D2D1::RectF(250, 330, 550, 375);

    const D2D1_RECT_F BTN_PAUSE_RESUME_RECT = D2D1::RectF(250, 270, 550, 310);
    const D2D1_RECT_F BTN_PAUSE_RESTART_RECT = D2D1::RectF(250, 320, 550, 360);
    const D2D1_RECT_F BTN_PAUSE_TITLE_RECT = D2D1::RectF(250, 370, 550, 410);

   

    struct FloatingText {
        wchar_t text[32];
        float x, y;
        float alpha;
        float timer;
        bool active;
    };

    struct Enemy {
        EnemyType type;
        float x, y;
        float vx, vy;
        float radius;
        int hp;
        int maxHp;
        bool isHit;
        bool active;
        int chainHitCount;
    };

    struct WhiteStone {
        float x, y;
        float vx, vy;
        float radius;
        float angle;

        bool isFired;
        bool isCooldown;
        float cooldownTimer;
        float blinkTimer;
    };

    GameState g_gameState = STATE_TITLE;

    WhiteStone g_white;
    std::vector<Enemy> g_enemies;
    std::vector<FloatingText> g_floatingTexts;

    int g_score = 0;
    float g_gameTimer = GAME_TIME_LIMIT;
    float g_blackSpawnTimer = 0.0f;

    std::vector<int> g_topScores = { 0, 0, 0 };

    bool g_keyLeft = false;
    bool g_keyRight = false;

    ID2D1Factory* g_pD2DFactory = nullptr;
    ID2D1HwndRenderTarget* g_pRenderTarget = nullptr;
    IDWriteFactory* g_pDWriteFactory = nullptr;
    IDWriteTextFormat* g_pTextFormat = nullptr;

    HRESULT CreateDeviceIndependentResources() {
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
        if (SUCCEEDED(hr)) {
            hr = DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&g_pDWriteFactory)
            );
        }
        if (SUCCEEDED(hr)) {
            hr = g_pDWriteFactory->CreateTextFormat(
                L"맑은 고딕",
                NULL,
                DWRITE_FONT_WEIGHT_BOLD,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                18.0f,
                L"ko-kr",
                &g_pTextFormat
            );
        }
        return hr;
    }

    HRESULT CreateDeviceResources(HWND hwnd) {
        HRESULT hr = S_OK;
        if (!g_pRenderTarget) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

            hr = g_pD2DFactory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(hwnd, size),
                &g_pRenderTarget
            );
        }
        return hr;
    }

    void DiscardDeviceResources() {
        if (g_pRenderTarget) {
            g_pRenderTarget->Release();
            g_pRenderTarget = nullptr;
        }
    }

  

    void AddFloatingText(float x, float y, const wchar_t* text) {
        FloatingText ft;
        swprintf_s(ft.text, 32, L"%s", text);
        ft.x = x;
        ft.y = y;
        ft.alpha = 1.0f;
        ft.timer = 0.8f;
        ft.active = true;
        g_floatingTexts.push_back(ft);
    }

    void ResetWhiteStone() {
        g_white.x = BOARD_START_X + BOARD_WIDTH / 2.0f;
        g_white.y = BOARD_START_Y + BOARD_HEIGHT - 20.0f;
        g_white.vx = 0.0f;
        g_white.vy = 0.0f;
        g_white.radius = 12.0f;
        g_white.isFired = false;
        g_white.isCooldown = false;
        g_white.cooldownTimer = 0.0f;
        g_white.blinkTimer = 0.0f;
    }

    void InitGame() {
        g_score = 0;
        g_gameTimer = GAME_TIME_LIMIT;
        g_blackSpawnTimer = 0.0f;
        g_enemies.clear();
        g_floatingTexts.clear();
        g_keyLeft = false;
        g_keyRight = false;
        g_white.angle = -PI / 2.0f;

        ResetWhiteStone();
    }

    void StartCooldown(float duration) {
        g_white.isFired = false;
        g_white.isCooldown = true;
        g_white.cooldownTimer = duration;
        g_white.blinkTimer = 0.0f;
        g_white.vx = 0.0f;
        g_white.vy = 0.0f;
    }

    void SpawnEnemy() {
        Enemy e;
        int randomCol = rand() % NUM_COLS;
        e.x = BOARD_START_X + randomCol * CELL_SIZE_PX;
        e.y = BOARD_START_Y;
        e.isHit = false;
        e.active = true;
        e.chainHitCount = 1;

        int randVal = rand() % 100;
        if (randVal < 60) {
            e.type = TYPE_BLACK_STONE;
            e.radius = 12.0f;
            e.hp = 1;
            e.maxHp = 1;
            e.vx = 0.0f;
            e.vy = 80.0f;
        }
        else if (randVal < 85) {
            e.type = TYPE_JANGGI_MA;
            e.radius = 15.0f;
            e.hp = 1;
            e.maxHp = 1;
            e.vx = (rand() % 2 == 0) ? 110.0f : -110.0f;
            e.vy = 110.0f;
        }
        else {
            e.type = TYPE_JANGGI_KING;
            e.radius = 22.0f;
            e.hp = 3;
            e.maxHp = 3;
            e.vx = 0.0f;
            e.vy = 50.0f;
        }

        g_enemies.push_back(e);
    }

    void UpdateGame(float dt) {
        if (g_gameState != STATE_PLAYING) return;

        g_gameTimer -= dt;
        if (g_gameTimer <= 0.0f) {
            g_gameTimer = 0.0f;
            g_gameState = STATE_GAMEOVER;
            return;
        }

        for (auto& ft : g_floatingTexts) {
            if (!ft.active) continue;
            ft.y -= 30.0f * dt;
            ft.timer -= dt;
            ft.alpha = max(0.0f, ft.timer / 0.8f);
            if (ft.timer <= 0.0f) ft.active = false;
        }

        if (g_white.isCooldown) {
            g_white.cooldownTimer -= dt;
            g_white.blinkTimer += dt;
            if (g_white.cooldownTimer <= 0.0f) {
                ResetWhiteStone();
            }
        }
        else if (!g_white.isFired) {
            // 회전 속도를 기존 2.5f에서 4.5f로 가속 (빠른 좌우 조정)
            float rotateSpeed = 4.5f * dt;
            if (g_keyLeft)  g_white.angle -= rotateSpeed;
            if (g_keyRight) g_white.angle += rotateSpeed;

            if (g_white.angle < -PI + 0.3f) g_white.angle = -PI + 0.3f;
            if (g_white.angle > -0.3f)      g_white.angle = -0.3f;
        }
        else {
            g_white.x += g_white.vx * dt;
            g_white.y += g_white.vy * dt;

            if (g_white.x < BOARD_START_X || g_white.x > BOARD_START_X + BOARD_WIDTH ||
                g_white.y < BOARD_START_Y || g_white.y > BOARD_START_Y + BOARD_HEIGHT) {
                StartCooldown(1.5f);
            }
        }

        g_blackSpawnTimer += dt;
        if (g_blackSpawnTimer >= 0.7f) {
            g_blackSpawnTimer = 0.0f;
            SpawnEnemy();
        }

        for (auto& e : g_enemies) {
            if (!e.active) continue;

            e.x += e.vx * dt;
            e.y += e.vy * dt;

            if (!e.isHit && e.type == TYPE_JANGGI_MA) {
                if (e.x - e.radius < BOARD_START_X) {
                    e.x = BOARD_START_X + e.radius;
                    e.vx = -e.vx;
                }
                else if (e.x + e.radius > BOARD_START_X + BOARD_WIDTH) {
                    e.x = BOARD_START_X + BOARD_WIDTH - e.radius;
                    e.vx = -e.vx;
                }
            }

            if (e.y > BOARD_START_Y + BOARD_HEIGHT + 50 || e.y < BOARD_START_Y - 50) {
                e.active = false;
            }
        }

        if (g_white.isFired && !g_white.isCooldown) {
            for (auto& e : g_enemies) {
                if (!e.active || e.isHit) continue;

                float dx = e.x - g_white.x;
                float dy = e.y - g_white.y;
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist <= (g_white.radius + e.radius) && dist > 0.0001f) {
                    float nx = dx / dist;
                    float ny = dy / dist;

                    e.hp--;

                    if (e.hp <= 0) {
                        e.isHit = true;

                        float whiteSpeed = sqrtf(g_white.vx * g_white.vx + g_white.vy * g_white.vy);
                        float pushSpeed = max(whiteSpeed * 1.2f, 400.0f);

                        e.vx = nx * pushSpeed;
                        e.vy = ny * pushSpeed;

                        if (e.type == TYPE_BLACK_STONE) g_score += 1;
                        else if (e.type == TYPE_JANGGI_MA) g_score += 2;
                        else if (e.type == TYPE_JANGGI_KING) g_score += 5;
                    }
                    else {
                        e.y -= 10.0f;
                        g_score += 5;
                    }

                    ResetWhiteStone();
                    break;
                }
            }
        }

        for (size_t i = 0; i < g_enemies.size(); ++i) {
            if (!g_enemies[i].active || !g_enemies[i].isHit || g_enemies[i].type != TYPE_JANGGI_KING) continue;

            for (size_t j = 0; j < g_enemies.size(); ++j) {
                if (i == j || !g_enemies[j].active || g_enemies[j].isHit) continue;

                float dx = g_enemies[j].x - g_enemies[i].x;
                float dy = g_enemies[j].y - g_enemies[i].y;
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist <= (g_enemies[i].radius + g_enemies[j].radius) && dist > 0.0001f) {
                    float nx = dx / dist;
                    float ny = dy / dist;

                    g_enemies[j].isHit = true;
                    float kingSpeed = sqrtf(g_enemies[i].vx * g_enemies[i].vx + g_enemies[i].vy * g_enemies[i].vy);
                    float pushSpeed = max(kingSpeed * 0.9f, 350.0f);

                    g_enemies[j].vx = nx * pushSpeed;
                    g_enemies[j].vy = ny * pushSpeed;

                    g_score += 75;

                    g_enemies[i].chainHitCount++;
                    wchar_t textBuf[32];
                    swprintf_s(textBuf, 32, L"%d연타! +75", g_enemies[i].chainHitCount);
                    AddFloatingText(g_enemies[j].x, g_enemies[j].y, textBuf);
                }
            }
        }
    }

    void RenderButtonD2D(D2D1_RECT_F rect, const wchar_t* text, D2D1_COLOR_F bgColor) {
        ID2D1SolidColorBrush* pBrush = nullptr;
        g_pRenderTarget->CreateSolidColorBrush(bgColor, &pBrush);
        g_pRenderTarget->FillRectangle(rect, pBrush);

        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
        g_pRenderTarget->DrawRectangle(rect, pBrush, 2.0f);

        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        g_pRenderTarget->DrawTextW(text, (UINT32)wcslen(text), g_pTextFormat, rect, pBrush);
        pBrush->Release();
    }

    void DrawArrow(float startX, float startY, float angle, float length, ID2D1SolidColorBrush* pBrush) {
        float endX = startX + cosf(angle) * length;
        float endY = startY + sinf(angle) * length;

        g_pRenderTarget->DrawLine(D2D1::Point2F(startX, startY), D2D1::Point2F(endX, endY), pBrush, 3.0f);

        float arrowHeadSize = 15.0f;
        float angle1 = angle + PI - 0.5f;
        float angle2 = angle + PI + 0.5f;

        D2D1_POINT_2F p1 = D2D1::Point2F(endX + cosf(angle1) * arrowHeadSize, endY + sinf(angle1) * arrowHeadSize);
        D2D1_POINT_2F p2 = D2D1::Point2F(endX + cosf(angle2) * arrowHeadSize, endY + sinf(angle2) * arrowHeadSize);

        g_pRenderTarget->DrawLine(D2D1::Point2F(endX, endY), p1, pBrush, 3.0f);
        g_pRenderTarget->DrawLine(D2D1::Point2F(endX, endY), p2, pBrush, 3.0f);
    }
    void DrawOctagonEnemy(float centerX, float centerY, float radius, const wchar_t* symbol, int hp, int maxHp, ID2D1SolidColorBrush* pBrush) {
        // 1. 말 배경 테두리 및 본체 (둥근 사각형 또는 원형)
        D2D1_RECT_F pieceRect = D2D1::RectF(centerX - radius, centerY - radius, centerX + radius, centerY + radius);

        // 말 종류에 따른 색상 설정 (마: 주황빛, 왕: 붉은빛)
        if (wcscmp(symbol, L"馬") == 0) {
            pBrush->SetColor(D2D1::ColorF(210.0f / 255.0f, 160.0f / 255.0f, 100.0f / 255.0f));
        }
        else {
            pBrush->SetColor(D2D1::ColorF(180.0f / 255.0f, 50.0f / 255.0f, 50.0f / 255.0f));
        }

        // 사각형 본체 채우기
        g_pRenderTarget->FillRectangle(pieceRect, pBrush);

        // 테두리 그리기
        pBrush->SetColor(D2D1::ColorF(80.0f / 255.0f, 40.0f / 255.0f, 10.0f / 255.0f));
        g_pRenderTarget->DrawRectangle(pieceRect, pBrush, 2.0f);

        // 2. 글자(馬, 王) 출력
        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        g_pRenderTarget->DrawTextW(symbol, 1, g_pTextFormat, pieceRect, pBrush);

        // 3. 체력(HP)바 표시 (왕처럼 피가 여러 개인 경우)
        if (maxHp > 1) {
            float barWidth = radius * 2.0f;
            float barHeight = 4.0f;
            float barX = centerX - radius;
            float barY = centerY - radius - 10.0f;

            pBrush->SetColor(D2D1::ColorF(0.1f, 0.1f, 0.1f, 0.8f));
            g_pRenderTarget->FillRectangle(D2D1::RectF(barX, barY, barX + barWidth, barY + barHeight), pBrush);

            float hpRatio = (float)hp / (float)maxHp;
            pBrush->SetColor(D2D1::ColorF(1.0f - hpRatio, hpRatio, 0.2f));
            g_pRenderTarget->FillRectangle(D2D1::RectF(barX, barY, barX + (barWidth * hpRatio), barY + barHeight), pBrush);

            pBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f));
            g_pRenderTarget->DrawRectangle(D2D1::RectF(barX, barY, barX + barWidth, barY + barHeight), pBrush, 1.0f);
        }
    

        /* ID2D1PathGeometry* pPathGeometry = nullptr;
        ID2D1GeometrySink* pSink = nullptr;

        if (SUCCEEDED(g_pD2DFactory->CreatePathGeometry(&pPathGeometry))) {
            if (SUCCEEDED(pPathGeometry->Open(&pSink))) {
                pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

                for (int i = 0; i < 8; ++i) {
                    float angle = (i * PI / 4.0f) + (PI / 8.0f);
                    float px = centerX + cosf(angle) * radius;
                    float py = centerY + sinf(angle) * radius;

                    if (i == 0) pSink->BeginFigure(D2D1::Point2F(px, py), D2D1_FIGURE_BEGIN_FILLED);
                    else pSink->AddLine(D2D1::Point2F(px, py));
                }
                pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
                pSink->Close();

                pBrush->SetColor(D2D1::ColorF(210.0f / 255.0f, 160.0f / 255.0f, 100.0f / 255.0f));
                g_pRenderTarget->FillGeometry(pPathGeometry, pBrush);

                pBrush->SetColor(D2D1::ColorF(80.0f / 255.0f, 40.0f / 255.0f, 10.0f / 255.0f));
                g_pRenderTarget->DrawGeometry(pPathGeometry, pBrush, 2.0f);

                pSink->Release();
            }
            pPathGeometry->Release();
        }

        pBrush->SetColor(D2D1::ColorF(180.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f));
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        D2D1_RECT_F textRect = D2D1::RectF(centerX - radius, centerY - radius, centerX + radius, centerY + radius);
        g_pRenderTarget->DrawTextW(symbol, 1, g_pTextFormat, textRect, pBrush);

        if (maxHp > 1) {
            float barWidth = radius * 2.0f;
            float barHeight = 4.0f;
            float barX = centerX - radius;
            float barY = centerY - radius - 10.0f;

            pBrush->SetColor(D2D1::ColorF(0.1f, 0.1f, 0.1f, 0.8f));
            g_pRenderTarget->FillRectangle(D2D1::RectF(barX, barY, barX + barWidth, barY + barHeight), pBrush);

            float hpRatio = (float)hp / (float)maxHp;
            pBrush->SetColor(D2D1::ColorF(1.0f - hpRatio, hpRatio, 0.2f));
            g_pRenderTarget->FillRectangle(D2D1::RectF(barX, barY, barX + (barWidth * hpRatio), barY + barHeight), pBrush);

            pBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f));
            g_pRenderTarget->DrawRectangle(D2D1::RectF(barX, barY, barX + barWidth, barY + barHeight), pBrush, 1.0f);

            wchar_t hpBuf[8];
            swprintf_s(hpBuf, 8, L"%d", hp);

            pBrush->SetColor(D2D1::ColorF(1.0f, 0.3f, 0.3f));
            D2D1_RECT_F numRect = D2D1::RectF(centerX + radius - 4.0f, centerY - radius - 15.0f, centerX + radius + 16.0f, centerY - radius + 5.0f);
            g_pRenderTarget->DrawTextW(hpBuf, (UINT32)wcslen(hpBuf), g_pTextFormat, numRect, pBrush);
        }*/
    }

    void RenderGameScreenD2D() {
        ID2D1SolidColorBrush* pBrush = nullptr;
        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);

        D2D1_RECT_F boardRect = D2D1::RectF(BOARD_START_X, BOARD_START_Y, BOARD_START_X + BOARD_WIDTH, BOARD_START_Y + BOARD_HEIGHT);
        pBrush->SetColor(D2D1::ColorF(240.0f / 255.0f, 215.0f / 255.0f, 155.0f / 255.0f));
        g_pRenderTarget->FillRectangle(boardRect, pBrush);

        pBrush->SetColor(D2D1::ColorF(60.0f / 255.0f, 50.0f / 255.0f, 40.0f / 255.0f));
        for (int i = 0; i < NUM_COLS; ++i) {
            float x = BOARD_START_X + i * CELL_SIZE_PX;
            g_pRenderTarget->DrawLine(D2D1::Point2F(x, BOARD_START_Y), D2D1::Point2F(x, BOARD_START_Y + BOARD_HEIGHT), pBrush, 1.0f);
        }
        for (int j = 0; j < NUM_ROWS; ++j) {
            float y = BOARD_START_Y + j * CELL_SIZE_PX;
            g_pRenderTarget->DrawLine(D2D1::Point2F(BOARD_START_X, y), D2D1::Point2F(BOARD_START_X + BOARD_WIDTH, y), pBrush, 1.0f);
        }

        for (const auto& e : g_enemies) {
            if (!e.active) continue;

            if (e.type == TYPE_BLACK_STONE) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(e.x, e.y), e.radius, e.radius);
                pBrush->SetColor(D2D1::ColorF(0.05f, 0.05f, 0.05f));
                g_pRenderTarget->FillEllipse(ellipse, pBrush);
            }
            else {
                const wchar_t* symbol = (e.type == TYPE_JANGGI_MA) ? L"馬" : L"王";
                DrawOctagonEnemy(e.x, e.y, e.radius, symbol, e.hp, e.maxHp, pBrush);
            }
        }

        bool shouldDrawWhite = true;
        if (g_white.isCooldown) {
            if ((int)(g_white.blinkTimer * 10.0f) % 2 == 0) shouldDrawWhite = false;
        }

        if (shouldDrawWhite) {
            D2D1_ELLIPSE whiteEllipse = D2D1::Ellipse(D2D1::Point2F(g_white.x, g_white.y), g_white.radius, g_white.radius);
            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
            g_pRenderTarget->FillEllipse(whiteEllipse, pBrush);

            if (!g_white.isFired && !g_white.isCooldown) {
                pBrush->SetColor(D2D1::ColorF(1.0f, 0.2f, 0.2f));
                DrawArrow(g_white.x, g_white.y, g_white.angle, 70.0f, pBrush);
            }
        }

        // 연타 플로팅 텍스트 렌더링 (나무판 위에서 잘 보이도록 짙은 파란색으로 변경)
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        for (const auto& ft : g_floatingTexts) {
            if (!ft.active) continue;
            pBrush->SetColor(D2D1::ColorF(0.0f, 0.2f, 0.6f, ft.alpha)); // 짙은 파란색
            D2D1_RECT_F textRect = D2D1::RectF(ft.x - 60.0f, ft.y - 15.0f, ft.x + 60.0f, ft.y + 15.0f);
            g_pRenderTarget->DrawTextW(ft.text, (UINT32)wcslen(ft.text), g_pTextFormat, textRect, pBrush);
        }

        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        wchar_t timeStr[32];
        swprintf_s(timeStr, 32, L"TIME: %.1fs", g_gameTimer);
        g_pRenderTarget->DrawTextW(timeStr, (UINT32)wcslen(timeStr), g_pTextFormat, D2D1::RectF(0, 310, WINDOW_WIDTH, 340), pBrush);

        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        wchar_t scoreStr[64];
        swprintf_s(scoreStr, 64, L"SCORE: %d", g_score);
        g_pRenderTarget->DrawTextW(scoreStr, (UINT32)wcslen(scoreStr), g_pTextFormat, D2D1::RectF(110, 320, 400, 350), pBrush);

        pBrush->Release();
    }

    void RenderTitleScreenD2D() {
        ID2D1SolidColorBrush* pBrush = nullptr;
        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.85f, 0.4f), &pBrush);
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pRenderTarget->DrawTextW(L"STONE SHOOTER", 13, g_pTextFormat, D2D1::RectF(0, 100, WINDOW_WIDTH, 140), pBrush);
        pBrush->Release();

        // 랭킹 렌더링 삭제 완료
        RenderButtonD2D(BTN_START_RECT, L"START GAME", D2D1::ColorF(0.15f, 0.55f, 0.27f));
        RenderButtonD2D(BTN_EXIT_RECT, L"END RELAY", D2D1::ColorF(0.70f, 0.24f, 0.24f));

        // 조작법 렌더링
        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.7f, 0.7f), &pBrush);
        g_pRenderTarget->DrawTextW(L"[LEFT/RIGHT] AIM   |   [SPACE] SHOOT", 38, g_pTextFormat, D2D1::RectF(0, 420, WINDOW_WIDTH, 450), pBrush);
        pBrush->Release();
    }

    void RenderPauseOverlayD2D() {
        ID2D1SolidColorBrush* pBrush = nullptr;

        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.04f, 0.05f, 0.07f, 0.8f), &pBrush);
        g_pRenderTarget->FillRectangle(D2D1::RectF(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), pBrush);

        pBrush->SetColor(D2D1::ColorF(1.0f, 0.85f, 0.4f));
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pRenderTarget->DrawTextW(L"PAUSED (일시정지)", 13, g_pTextFormat, D2D1::RectF(0, 30, WINDOW_WIDTH, 70), pBrush);

        pBrush->SetColor(D2D1::ColorF(0.4f, 0.85f, 1.0f));
        wchar_t curStr[64];
        swprintf_s(curStr, 64, L"현재 점수 : %d 점 (남은시간: %.1fs)", g_score, g_gameTimer);
        g_pRenderTarget->DrawTextW(curStr, (UINT32)wcslen(curStr), g_pTextFormat, D2D1::RectF(0, 80, WINDOW_WIDTH, 105), pBrush);

        pBrush->SetColor(D2D1::ColorF(1.0f, 0.84f, 0.0f));
        g_pRenderTarget->DrawTextW(L"🏆 TOP 3 HIGH SCORES 🏆", 20, g_pTextFormat, D2D1::RectF(0, 115, WINDOW_WIDTH, 140), pBrush);

        pBrush->SetColor(D2D1::ColorF(0.8f, 0.8f, 0.8f));
        for (int i = 0; i < 3; ++i) {
            wchar_t buf[64];
            swprintf_s(buf, 64, L"%d위 : %d 점", i + 1, g_topScores[i]);
            g_pRenderTarget->DrawTextW(buf, (UINT32)wcslen(buf), g_pTextFormat, D2D1::RectF(0, 145 + i * 25, WINDOW_WIDTH, 170 + i * 25), pBrush);
        }

        pBrush->Release();

        RenderButtonD2D(BTN_PAUSE_RESUME_RECT, L"계 속 하 기 (ESC)", D2D1::ColorF(0.15f, 0.55f, 0.27f));
        RenderButtonD2D(BTN_PAUSE_RESTART_RECT, L"다 시 하 기", D2D1::ColorF(0.20f, 0.40f, 0.70f));
        RenderButtonD2D(BTN_PAUSE_TITLE_RECT, L"첫화면으로 돌아가기", D2D1::ColorF(0.70f, 0.24f, 0.24f));
    }

    void Render(HWND hwnd) {
        if (FAILED(CreateDeviceResources(hwnd))) return;

        g_pRenderTarget->BeginDraw();
        g_pRenderTarget->Clear(D2D1::ColorF(0.12f, 0.12f, 0.12f));

        // ⭐ 1280x720 기준, Game D(800x450)를 정중앙으로 이동시키는 매트릭스 적용!
        D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Translation((1280.0f - WINDOW_WIDTH) / 2.0f, (720.0f - WINDOW_HEIGHT) / 2.0f);
        g_pRenderTarget->SetTransform(transform);

        if (g_gameState == STATE_PLAYING) RenderGameScreenD2D();
        else if (g_gameState == STATE_PAUSED) { RenderGameScreenD2D(); RenderPauseOverlayD2D(); }
        else if (g_gameState == STATE_TITLE) RenderTitleScreenD2D();
        else if (g_gameState == STATE_GAMEOVER) RenderButtonD2D(BTN_RESTART_RECT, L"다음에 계속 (SPACE)", D2D1::ColorF(0.15f, 0.55f, 0.27f));

        // ⭐ 그림 다 그리고 나면 변환 매트릭스 원상복구!
        g_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        HRESULT hr = g_pRenderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) DiscardDeviceResources();
    }

    bool IsPointInRectF(POINT pt, D2D1_RECT_F rect) {
        return (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom);
    }
    bool gameoverchecker = false;

    void Init(HWND hWnd) {
        g_pRenderTarget = ::g_pRenderTarget;
        if (!g_pDWriteFactory) CreateDeviceIndependentResources();

        gameoverchecker = false;      // 바통 초기화
        g_gameState = STATE_TITLE;    // 무조건 내부 타이틀부터 시작!
        InitGame();
    }

    void Update() {
        UpdateGame(0.016f);
    }

    void Draw() {
        if (g_gameState == STATE_PLAYING) RenderGameScreenD2D();
        else if (g_gameState == STATE_PAUSED) { RenderGameScreenD2D(); RenderPauseOverlayD2D(); }
        else if (g_gameState == STATE_TITLE) RenderTitleScreenD2D();
        else if (g_gameState == STATE_GAMEOVER) RenderButtonD2D(BTN_RESTART_RECT, L"다음에 계속 (SPACE)", D2D1::ColorF(0.15f, 0.55f, 0.27f));
    }

    void Release() {
        if (g_pTextFormat) g_pTextFormat->Release();
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
    }

    // ⭐ 바통을 넘길 때만 true 반환 (시간 끝났다고 바로 넘기지 않음)
    bool IsGameOver() {
        return gameoverchecker;
    }

    int GetScore() {
        return g_score;
    }

    void InputMouseClick(int x, int y) {
        if (g_gameState == STATE_PLAYING) return;
        POINT pt = { x, y };

        if (g_gameState == STATE_TITLE) {
            if (IsPointInRectF(pt, BTN_START_RECT)) {
                InitGame();
                g_gameState = STATE_PLAYING;
            }
            else if (IsPointInRectF(pt, BTN_EXIT_RECT)) {
                // ⭐ 타이틀에서 종료를 누르면 릴레이(네임 인풋)로 탈출!
                gameoverchecker = true;
            }
        }
        else if (g_gameState == STATE_GAMEOVER) {
            if (IsPointInRectF(pt, BTN_RESTART_RECT)) {
                // ⭐ 게임오버 화면에서 버튼을 누르면 다음 릴레이로 탈출!
                gameoverchecker = true;
            }
        }
    }

    // ⭐ 키보드(Space/Enter)로도 씬을 부드럽게 넘길 수 있도록 보강!
    void InputKey(WPARAM wParam) {
        if (g_gameState == STATE_TITLE) {
            if (wParam == VK_SPACE || wParam == VK_RETURN) {
                InitGame();
                g_gameState = STATE_PLAYING;
            }
        }
        else if (g_gameState == STATE_GAMEOVER) {
            if (wParam == VK_SPACE || wParam == VK_RETURN) {
                gameoverchecker = true; // 다음 릴레이(네임 인풋)로 바통 터치
            }
        }
        else if (g_gameState == STATE_PLAYING) {
            if (wParam == VK_LEFT) g_keyLeft = true;
            if (wParam == VK_RIGHT) g_keyRight = true;
            if (wParam == VK_SPACE) {
                if (!g_white.isFired && !g_white.isCooldown) {
                    g_white.isFired = true;
                    float speed = 500.0f;
                    g_white.vx = cosf(g_white.angle) * speed;
                    g_white.vy = sinf(g_white.angle) * speed;
                }
            }
        }
    }

    void InputKeyUp(WPARAM wParam) {
        if (wParam == VK_LEFT) g_keyLeft = false;
        if (wParam == VK_RIGHT) g_keyRight = false;
    }
}