#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <mmsystem.h>
#include <cmath>
#include <algorithm>
#include <functional>
#include <thread>
#include "SoundManager.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "winmm.lib") // ⭐ MIDI 사운드용 라이브러리 필수

extern ID2D1HwndRenderTarget* g_pRenderTarget;

namespace GameD {
    // ==========================================
    // [MIDI Native Synth 클래스]
    // ==========================================
  






    int g_comboCount = 0;

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

    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 450;

    const int BOARD_WIDTH = 600;
    const int BOARD_HEIGHT = 360;
    const float BOARD_START_X = (WINDOW_WIDTH - BOARD_WIDTH) / 2.0f;
    const float BOARD_START_Y = 0.0f;

    const int NUM_ROWS = 15;
    const float CELL_SIZE_PX = (float)BOARD_HEIGHT / (NUM_ROWS - 1);
    const int NUM_COLS = (int)(BOARD_WIDTH / CELL_SIZE_PX) + 1;

    const float PI = 3.14159265f;
    const float GAME_TIME_LIMIT = 60.0f;

    const int MAX_ENEMIES = 50;
    const int MAX_FLOATING_TEXTS = 20;

    const D2D1_RECT_F BTN_START_RECT = D2D1::RectF(250, 320, 550, 360);
    const D2D1_RECT_F BTN_EXIT_RECT = D2D1::RectF(250, 375, 550, 415);
    const D2D1_RECT_F BTN_RESTART_RECT = D2D1::RectF(250, 375, 550, 415); // 다음에 계속 (게임오버시)

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
        float dirChangeTimer;
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

    Enemy g_enemyPool[MAX_ENEMIES];
    FloatingText g_floatingTextPool[MAX_FLOATING_TEXTS];

    int g_score = 0;
    float g_gameTimer = GAME_TIME_LIMIT;
    float g_blackSpawnTimer = 0.0f;

    bool g_keyLeft = false;
    bool g_keyRight = false;
    bool gameoverchecker = false;

    ID2D1Factory* g_pD2DFactory = nullptr;
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

    void AddFloatingText(float x, float y, const wchar_t* text) {
        for (int i = 0; i < MAX_FLOATING_TEXTS; ++i) {
            if (!g_floatingTextPool[i].active) {
                swprintf_s(g_floatingTextPool[i].text, 32, L"%s", text);
                g_floatingTextPool[i].x = x;
                g_floatingTextPool[i].y = y;
                g_floatingTextPool[i].alpha = 1.0f;
                g_floatingTextPool[i].timer = 0.8f;
                g_floatingTextPool[i].active = true;
                break;
            }
        }
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
        g_comboCount = 0;
        g_gameTimer = GAME_TIME_LIMIT;
        g_blackSpawnTimer = 0.0f;

        memset(g_enemyPool, 0, sizeof(g_enemyPool));
        memset(g_floatingTextPool, 0, sizeof(g_floatingTextPool));

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
        int slot = -1;
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (!g_enemyPool[i].active) {
                slot = i;
                break;
            }
        }
        if (slot == -1) return;

        Enemy& e = g_enemyPool[slot];
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
            e.vx = (rand() % 2 == 0) ? 120.0f : -120.0f;
            e.vy = 100.0f;
            e.dirChangeTimer = 0.5f + (float)(rand() % 100) / 100.0f;
        }
        else {
            e.type = TYPE_JANGGI_KING;
            e.radius = 22.0f;
            e.hp = 3;
            e.maxHp = 3;
            e.vx = 0.0f;
            e.vy = 50.0f;
        }
    }

    void UpdateGame(float dt) {
        if (g_gameState != STATE_PLAYING) return;

        g_gameTimer -= dt;
        if (g_gameTimer <= 0.0f) {
            g_gameTimer = 0.0f;
            g_gameState = STATE_GAMEOVER;
            return;
        }

        for (int i = 0; i < MAX_FLOATING_TEXTS; ++i) {
            if (!g_floatingTextPool[i].active) continue;
            g_floatingTextPool[i].y -= 30.0f * dt;
            g_floatingTextPool[i].timer -= dt;
            g_floatingTextPool[i].alpha = (std::max)(0.0f, g_floatingTextPool[i].timer / 0.8f);
            if (g_floatingTextPool[i].timer <= 0.0f) {
                g_floatingTextPool[i].active = false;
            }
        }

        if (g_white.isCooldown) {
            g_white.cooldownTimer -= dt;
            g_white.blinkTimer += dt;
            if (g_white.cooldownTimer <= 0.0f) ResetWhiteStone();
        }
        else if (!g_white.isFired) {
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
                SoundManager::PlayOutSFX(); // ⭐ 장외 이탈 사운드 재생!
                StartCooldown(1.0f);
            }
        }

        g_blackSpawnTimer += dt;
        if (g_blackSpawnTimer >= 0.7f) {
            g_blackSpawnTimer = 0.0f;
            SpawnEnemy();
        }

        for (int i = 0; i < MAX_ENEMIES; ++i) {
            Enemy& e = g_enemyPool[i];
            if (!e.active) continue;

            e.x += e.vx * dt;
            e.y += e.vy * dt;

            if (!e.isHit && e.type == TYPE_JANGGI_MA) {
                e.dirChangeTimer -= dt;
                if (e.dirChangeTimer <= 0.0f) {
                    e.vx = -e.vx;
                    e.dirChangeTimer = 0.5f + (float)(rand() % 100) / 100.0f;
                }
                if (e.x - e.radius < BOARD_START_X) {
                    e.x = BOARD_START_X + e.radius;
                    e.vx = fabsf(e.vx);
                }
                else if (e.x + e.radius > BOARD_START_X + BOARD_WIDTH) {
                    e.x = BOARD_START_X + BOARD_WIDTH - e.radius;
                    e.vx = -fabsf(e.vx);
                }
            }
            if (e.y > BOARD_START_Y + BOARD_HEIGHT + 50 || e.y < BOARD_START_Y - 50) e.active = false;
        }

        if (g_white.isFired && !g_white.isCooldown) {
            for (int i = 0; i < MAX_ENEMIES; ++i) {
                Enemy& e = g_enemyPool[i];
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
                        float pushSpeed = (std::max)(whiteSpeed * 1.2f, 400.0f);

                        e.vx = nx * pushSpeed;
                        e.vy = ny * pushSpeed;

                        if (e.type == TYPE_BLACK_STONE) g_score += 100;
                        else if (e.type == TYPE_JANGGI_MA) g_score += 250;
                        else if (e.type == TYPE_JANGGI_KING) g_score += 500;
                    }
                    else {
                        e.y -= 10.0f;
                        g_score += 50;
                    }

                    SoundManager::PlayHitSFX(g_comboCount); // ⭐ 콤보 타격 사운드!
                    g_comboCount++;

                    ResetWhiteStone();
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_ENEMIES; ++i) {
            Enemy& e1 = g_enemyPool[i];
            if (!e1.active || !e1.isHit || e1.type != TYPE_JANGGI_KING) continue;

            for (int j = 0; j < MAX_ENEMIES; ++j) {
                if (i == j) continue;
                Enemy& e2 = g_enemyPool[j];
                if (!e2.active || e2.isHit) continue;

                float dx = e2.x - e1.x;
                float dy = e2.y - e1.y;
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist <= (e1.radius + e2.radius) && dist > 0.0001f) {
                    float nx = dx / dist;
                    float ny = dy / dist;

                    e2.isHit = true;
                    float kingSpeed = sqrtf(e1.vx * e1.vx + e1.vy * e1.vy);
                    float pushSpeed = (std::max)(kingSpeed * 0.9f, 350.0f);

                    e2.vx = nx * pushSpeed;
                    e2.vy = ny * pushSpeed;

                    wchar_t textBuf[32];
                    if (e2.type == TYPE_JANGGI_KING) {
                        g_score += 150;
                        e1.chainHitCount++;
                        swprintf_s(textBuf, 32, L"장군! +150");
                    }
                    else {
                        g_score += 75;
                        e1.chainHitCount++;
                        swprintf_s(textBuf, 32, L"%d연타! +75", e1.chainHitCount);
                    }

                    SoundManager::PlayHitSFX(g_comboCount); // ⭐ 체인 히트 사운드!
                    g_comboCount++;

                    AddFloatingText(e2.x, e2.y, textBuf);
                }
            }
        }
    }

    // ==========================================
    // [렌더링 함수들]
    // ==========================================

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
        g_pRenderTarget->DrawLine(D2D1::Point2F(endX, endY), D2D1::Point2F(endX + cosf(angle1) * arrowHeadSize, endY + sinf(angle1) * arrowHeadSize), pBrush, 3.0f);
        g_pRenderTarget->DrawLine(D2D1::Point2F(endX, endY), D2D1::Point2F(endX + cosf(angle2) * arrowHeadSize, endY + sinf(angle2) * arrowHeadSize), pBrush, 3.0f);
    }

    void DrawSquareEnemy(float centerX, float centerY, float radius, const wchar_t* symbol, int hp, int maxHp, ID2D1SolidColorBrush* pBrush) {
        D2D1_RECT_F rect = D2D1::RectF(centerX - radius, centerY - radius, centerX + radius, centerY + radius);

        pBrush->SetColor(D2D1::ColorF(210.0f / 255.0f, 160.0f / 255.0f, 100.0f / 255.0f));
        g_pRenderTarget->FillRectangle(rect, pBrush);

        pBrush->SetColor(D2D1::ColorF(80.0f / 255.0f, 40.0f / 255.0f, 10.0f / 255.0f));
        g_pRenderTarget->DrawRectangle(rect, pBrush, 2.0f);

        pBrush->SetColor(D2D1::ColorF(180.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f));
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        g_pRenderTarget->DrawTextW(symbol, 1, g_pTextFormat, rect, pBrush);

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
            g_pRenderTarget->DrawTextW(hpBuf, (UINT32)wcslen(hpBuf), g_pTextFormat, D2D1::RectF(centerX + radius - 4.0f, centerY - radius - 15.0f, centerX + radius + 16.0f, centerY - radius + 5.0f), pBrush);
        }
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

        for (int i = 0; i < MAX_ENEMIES; ++i) {
            const Enemy& e = g_enemyPool[i];
            if (!e.active) continue;

            if (e.type == TYPE_BLACK_STONE) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(e.x, e.y), e.radius, e.radius);
                pBrush->SetColor(D2D1::ColorF(0.05f, 0.05f, 0.05f));
                g_pRenderTarget->FillEllipse(ellipse, pBrush);
            }
            else {
                const wchar_t* symbol = (e.type == TYPE_JANGGI_MA) ? L"馬" : L"王";
                DrawSquareEnemy(e.x, e.y, e.radius, symbol, e.hp, e.maxHp, pBrush);
            }
        }

        bool shouldDrawWhite = true;
        if (g_white.isCooldown && ((int)(g_white.blinkTimer * 10.0f) % 2 == 0)) shouldDrawWhite = false;

        if (shouldDrawWhite) {
            D2D1_ELLIPSE whiteEllipse = D2D1::Ellipse(D2D1::Point2F(g_white.x, g_white.y), g_white.radius, g_white.radius);
            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
            g_pRenderTarget->FillEllipse(whiteEllipse, pBrush);

            if (!g_white.isFired && !g_white.isCooldown) {
                pBrush->SetColor(D2D1::ColorF(1.0f, 0.2f, 0.2f));
                DrawArrow(g_white.x, g_white.y, g_white.angle, 70.0f, pBrush);
            }
        }

        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        for (int i = 0; i < MAX_FLOATING_TEXTS; ++i) {
            const FloatingText& ft = g_floatingTextPool[i];
            if (!ft.active) continue;
            pBrush->SetColor(D2D1::ColorF(0.0f, 0.2f, 0.6f, ft.alpha));
            D2D1_RECT_F textRect = D2D1::RectF(ft.x - 60.0f, ft.y - 15.0f, ft.x + 60.0f, ft.y + 15.0f);
            g_pRenderTarget->DrawTextW(ft.text, (UINT32)wcslen(ft.text), g_pTextFormat, textRect, pBrush);
        }

        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        wchar_t timeStr[32];
        swprintf_s(timeStr, 32, L"TIME: %.1fs", g_gameTimer);
        g_pRenderTarget->DrawTextW(timeStr, (UINT32)wcslen(timeStr), g_pTextFormat, D2D1::RectF(0, 380, WINDOW_WIDTH, 420), pBrush);

        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        wchar_t scoreStr[64];
        swprintf_s(scoreStr, 64, L"SCORE: %d", g_score);
        g_pRenderTarget->DrawTextW(scoreStr, (UINT32)wcslen(scoreStr), g_pTextFormat, D2D1::RectF(110, 380, 400, 420), pBrush);

        pBrush->Release();
    }

    void RenderTitleScreenD2D() {
        ID2D1SolidColorBrush* pBrush = nullptr;
        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.85f, 0.4f), &pBrush);
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_pRenderTarget->DrawTextW(L"STONE SHOOTER", 13, g_pTextFormat, D2D1::RectF(0, 100, WINDOW_WIDTH, 140), pBrush);
        pBrush->Release();

        // 릴레이 맞춤형 UI
        RenderButtonD2D(BTN_START_RECT, L"START GAME", D2D1::ColorF(0.15f, 0.55f, 0.27f));
        RenderButtonD2D(BTN_EXIT_RECT, L"END RELAY", D2D1::ColorF(0.70f, 0.24f, 0.24f));

        g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.7f, 0.7f), &pBrush);
        g_pRenderTarget->DrawTextW(L"[LEFT/RIGHT] AIM   |   [SPACE] SHOOT", 38, g_pTextFormat, D2D1::RectF(0, 420, WINDOW_WIDTH, 450), pBrush);
        pBrush->Release();
    }

    bool IsPointInRectF(POINT pt, D2D1_RECT_F rect) {
        return (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom);
    }

    // ==========================================
    // [프레임워크 연결부 (SceneManager 인터페이스)]
    // ==========================================
    void Init(HWND hWnd) {
        g_pRenderTarget = ::g_pRenderTarget;
        if (!g_pDWriteFactory) CreateDeviceIndependentResources();

        gameoverchecker = false;
        g_gameState = STATE_TITLE;
        InitGame();
    }

    void Update() {
        UpdateGame(0.016f);
    }

    void Draw() {
        if (g_gameState == STATE_PLAYING) RenderGameScreenD2D();
        else if (g_gameState == STATE_TITLE) RenderTitleScreenD2D();
        else if (g_gameState == STATE_GAMEOVER) RenderButtonD2D(BTN_RESTART_RECT, L"다음에 계속 (SPACE)", D2D1::ColorF(0.15f, 0.55f, 0.27f));
    }

    void Release() {
        if (g_pTextFormat) g_pTextFormat->Release();
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) { g_pD2DFactory->Release(); g_pD2DFactory = nullptr; }
    
    }

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
                gameoverchecker = true; // END RELAY
            }
        }
        else if (g_gameState == STATE_GAMEOVER) {
            if (IsPointInRectF(pt, BTN_RESTART_RECT)) {
                gameoverchecker = true; // 다음 게임(Game E)으로 릴레이 진행
            }
        }
    }

    void InputKey(WPARAM wParam) {
        if (g_gameState == STATE_TITLE) {
            if (wParam == VK_SPACE || wParam == VK_RETURN) {
                InitGame();
                g_gameState = STATE_PLAYING;
            }
        }
        else if (g_gameState == STATE_GAMEOVER) {
            if (wParam == VK_SPACE || wParam == VK_RETURN) {
                gameoverchecker = true;
                SoundManager::StopBGM();
                SoundManager::PlayFanfare_2();
            }
        }
        else if (g_gameState == STATE_PLAYING) {
            if (wParam == VK_LEFT) g_keyLeft = true;
            if (wParam == VK_RIGHT) g_keyRight = true;
            if (wParam == VK_SPACE) {
                if (!g_white.isFired && !g_white.isCooldown) {
                    g_white.isFired = true;
                    g_comboCount = 0; // 발사할 때 콤보 카운트 초기화!

                    // ⭐ 최신 밸런스 패치 속도 반영 (750.0f)
                    float speed = 750.0f;
                    g_white.vx = cosf(g_white.angle) * speed;
                    g_white.vy = sinf(g_white.angle) * speed;

                    SoundManager::PlayShootSFX(); // ⭐ 발사 사운드 재생!
                }
            }
        }
    }

    void InputKeyUp(WPARAM wParam) {
        if (wParam == VK_LEFT) g_keyLeft = false;
        if (wParam == VK_RIGHT) g_keyRight = false;
    }
}