#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <fstream>
#include <cassert>
#include "SoundManager.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// --- 해상도 및 화면 설정 ---
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 450;
const int VIRTUAL_WIDTH = 400;
const int VIRTUAL_HEIGHT = 225;

// ============================================================================
// [DATA STRUCTURES] 공통 데이터 구조체
// ============================================================================
struct AABB {
    float minX, minY, maxX, maxY;
};

struct Player {
    float x, y, radius;
    bool isDashing = false;
    float dashDuration = 0.0f;
    float dashCooldown = 0.0f;
};

enum EnemyType { ENEMY_CIRCLE, ENEMY_TRIANGLE, ENEMY_HEXAGON };
bool isForceEndRelay = false; // ⭐ 강제 종료 신호
struct Enemy {
    EnemyType type = ENEMY_CIRCLE;
    float x, y, radius = 6.0f;
    float vx, vy;
    bool isTargeting = false;
    float targetTimer = 0.0f;
    float targetDirX = 0.0f, targetDirY = 0.0f;

    float angle = 0.0f;
    float shootTimer = 2.5f;
    bool hasEntered = false;

    bool isPreparingExplosion = false;
    float explosionTimer = 0.0f;
    float hexPauseTimer = 4.0f;
    float savedVx = 0.0f, savedVy = 0.0f;
};

struct Bullet {
    float x, y;
    float vx, vy;
    float radius = 2.0f;
    float life = 1.0f;
};

struct StarItem {
    float x = 0.0f, y = 0.0f;
    float radius = 6.0f;
    bool active = false;
    float angle = 0.0f;
    float life = 10.0f;
};

struct SwordItem {
    float x = 0.0f, y = 0.0f;
    float radius = 6.0f;
    bool active = false;
    float angle = 0.0f;
    float life = 10.0f;
};

struct ShieldItem {
    float x = 0.0f, y = 0.0f;
    float radius = 6.0f;
    bool active = false;
    float angle = 0.0f;
    float life = 10.0f;
};

struct SmallPillar {
    AABB box;
    bool isWarning = false;
    bool isActive = false;
    float timer = 0.0f;
    int hp = 4;
};

enum GiantPillarState { GIANT_NONE, GIANT_WARNING, GIANT_EXTENDING, GIANT_SOLID };
enum GiantPillarType { PILLAR_STRAIGHT, PILLAR_CURVED, PILLAR_SHAPE };

struct GiantPillar {
    GiantPillarState state = GIANT_NONE;
    GiantPillarType type = PILLAR_STRAIGHT;
    float timer = 0.0f;
    float thickness = 18.0f;

    float angle = 0.0f;
    float dirX = 0.0f, dirY = 0.0f;
    float normX = 0.0f, normY = 0.0f;
    float currentLength = 0.0f;

    float p0x = 0.0f, p0y = 0.0f;
    float p1x = 0.0f, p1y = 0.0f;
    float p2x = 0.0f, p2y = 0.0f;

    int shapeType = 0;
    float shapeCenterX = 0.0f;
    float shapeCenterY = 0.0f;
    std::vector<std::pair<float, float>> shapeVertices;
};

struct Particle {
    float x, y, vx, vy;
    float life, maxLife;
    COLORREF color = RGB(100, 100, 120);
    bool isHeart = false;
};

struct RankEntry {
    char name[4];
    int score;
};

// ============================================================================
// [SYSTEM] 사운드 및 세이브/로드 시스템
// ============================================================================

bool gameoverchecker = false;





float g_HighScore = 0.0f;
std::vector<RankEntry> g_RankList;
int g_MouseX = 0;
int g_MouseY = 0;
float g_DeltaTime = 0.0f;
HDC g_RenderDC = NULL;
HINSTANCE hInst;
WCHAR szTitle[100] = L"1.44MB Retro Survival Game";
WCHAR szWindowClass[100] = L"RetroSurvivalGameClass";

void LoadRankings() {
    g_RankList.clear();
    std::ifstream file("rank.dat", std::ios::binary);
    if (file.is_open()) {
        int size = 0;
        file.read(reinterpret_cast<char*>(&size), sizeof(int));
        for (int i = 0; i < size; ++i) {
            RankEntry entry;
            file.read(reinterpret_cast<char*>(&entry), sizeof(RankEntry));
            g_RankList.push_back(entry);
        }
        file.close();
    }
    if (!g_RankList.empty()) {
        g_HighScore = (float)g_RankList[0].score;
    }
    else {
        g_HighScore = 0.0f;
    }
}

void SaveRankings() {
    std::ofstream file("rank.dat", std::ios::binary);
    if (file.is_open()) {
        int size = (int)g_RankList.size();
        file.write(reinterpret_cast<const char*>(&size), sizeof(int));
        for (const auto& entry : g_RankList) {
            file.write(reinterpret_cast<const char*>(&entry), sizeof(RankEntry));
        }
        file.close();
    }
}

void AddRank(const std::string& name, int score) {
    RankEntry newEntry;
    memset(newEntry.name, 0, sizeof(newEntry.name));
    strncpy_s(newEntry.name, name.c_str(), 3);
    newEntry.score = score;

    g_RankList.push_back(newEntry);
    std::sort(g_RankList.begin(), g_RankList.end(), [](const RankEntry& a, const RankEntry& b) {
        return a.score > b.score;
        });

    if (g_RankList.size() > 10) {
        g_RankList.resize(10);
    }

    if (!g_RankList.empty()) {
        g_HighScore = (float)g_RankList[0].score;
    }
    SaveRankings();
}

// ============================================================================
// [UTILS] 수학 유틸리티
// ============================================================================
float DistToSegment(float qx, float qy, float ax, float ay, float bx, float by, float& projX, float& projY) {
    float abx = bx - ax;
    float aby = by - ay;
    float lenSq = abx * abx + aby * aby;
    if (lenSq < 0.0001f) {
        projX = ax; projY = ay;
        return std::sqrt((qx - ax) * (qx - ax) + (qy - ay) * (qy - ay));
    }
    float t = ((qx - ax) * abx + (qy - ay) * aby) / lenSq;
    t = (std::max)(0.0f, (std::min)(1.0f, t));
    projX = ax + t * abx;
    projY = ay + t * aby;
    float dx = qx - projX;
    float dy = qy - projY;
    return std::sqrt(dx * dx + dy * dy);
}

void GetBezierPoint(float t, float p0x, float p0y, float p1x, float p1y, float p2x, float p2y, float& rx, float& ry) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    rx = uu * p0x + 2.0f * u * t * p1x + tt * p2x;
    ry = uu * p0y + 2.0f * u * t * p1y + tt * p2y;
}

// ============================================================================
// [RENDER UTILS] 드로우 및 그래픽 유틸리티
// ============================================================================
void DrawPixelGrid(HDC hdc, float cx, float cy, const int* grid, int gridW, int gridH, float pSize, COLORREF color, float angle = 0.0f, COLORREF secColor = 0, bool useSecColor = false) {
    float cosA = cosf(angle);
    float sinA = sinf(angle);

    HBRUSH brush1 = CreateSolidBrush(color);
    HBRUSH brush2 = useSecColor ? CreateSolidBrush(secColor) : NULL;

    float halfW = gridW * 0.5f;
    float halfH = gridH * 0.5f;

    for (int r = 0; r < gridH; ++r) {
        for (int c = 0; c < gridW; ++c) {
            int val = grid[r * gridW + c];
            if (val == 0) continue;

            float lx = (c - halfW + 0.5f) * pSize;
            float ly = (r - halfH + 0.5f) * pSize;

            float rx = lx * cosA - ly * sinA + cx;
            float ry = lx * sinA + ly * cosA + cy;

            RECT pr = {
                (long)(rx - pSize * 0.5f),
                (long)(ry - pSize * 0.5f),
                (long)(rx + pSize * 0.5f + 1),
                (long)(ry + pSize * 0.5f + 1)
            };

            if (val == 2 && useSecColor) {
                FillRect(hdc, &pr, brush2);
            }
            else {
                FillRect(hdc, &pr, brush1);
            }
        }
    }

    DeleteObject(brush1);
    if (brush2) DeleteObject(brush2);
}

void DrawPixelRectBorder(HDC hdc, float minX, float minY, float maxX, float maxY, float pSize, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);

    for (float x = minX; x <= maxX; x += pSize) {
        RECT r1 = { (long)x, (long)minY, (long)(x + pSize + 1), (long)(minY + pSize + 1) };
        RECT r2 = { (long)x, (long)(maxY - pSize), (long)(x + pSize + 1), (long)(maxY + 1) };
        FillRect(hdc, &r1, brush);
        FillRect(hdc, &r2, brush);
    }

    for (float y = minY; y <= maxY; y += pSize) {
        RECT r1 = { (long)minX, (long)y, (long)(minX + pSize + 1), (long)(y + pSize + 1) };
        RECT r2 = { (long)(maxX - pSize), (long)y, (long)(maxX + 1), (long)(maxY + 1) };
        FillRect(hdc, &r1, brush);
        FillRect(hdc, &r2, brush);
    }

    DeleteObject(brush);
}

void DrawPixelHeart(HDC hdc, float cx, float cy, COLORREF color, float offsetX = 0.0f, float offsetY = 0.0f, float rotAngle = 0.0f, int half = 0) {
    const int GRID_W = 8;
    const int GRID_H = 7;
    const int grid[GRID_H][GRID_W] = {
        {0, 1, 1, 0, 0, 1, 1, 0},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 1, 1, 1, 1, 0, 0},
        {0, 0, 0, 1, 1, 0, 0, 0}
    };

    float pSize = 1.0f;
    float cosA = cosf(rotAngle);
    float sinA = sinf(rotAngle);

    HBRUSH brush = CreateSolidBrush(color);

    for (int r = 0; r < GRID_H; ++r) {
        for (int c = 0; c < GRID_W; ++c) {
            if (grid[r][c] == 0) continue;

            if (half == 1 && c >= 4) continue;
            if (half == 2 && c < 4) continue;

            float lx = (c - 3.5f) * pSize;
            float ly = (r - 3.0f) * pSize;

            float rx = lx * cosA - ly * sinA + cx + offsetX;
            float ry = lx * sinA + ly * cosA + cy + offsetY;

            RECT pr = {
                (long)(rx - pSize * 0.5f),
                (long)(ry - pSize * 0.5f),
                (long)(rx + pSize * 0.5f + 1),
                (long)(ry + pSize * 0.5f + 1)
            };
            FillRect(hdc, &pr, brush);
        }
    }

    DeleteObject(brush);
}

void DrawPixelChar(HDC hdc, char ch, float x, float y, float pSize, COLORREF color) {
    static const int fontG[5][5] = { {0,1,1,1,1},{1,0,0,0,0},{1,0,1,1,1},{1,0,0,0,1},{0,1,1,1,1} };
    static const int fontA[5][5] = { {0,1,1,1,0},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1} };
    static const int fontM[5][5] = { {1,0,0,0,1},{1,1,0,1,1},{1,0,1,0,1},{1,0,0,0,1},{1,0,0,0,1} };
    static const int fontE[5][5] = { {1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,0},{1,1,1,1,1} };
    static const int fontO[5][5] = { {0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0} };
    static const int fontV[5][5] = { {1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,0,1,0},{0,0,1,0,0} };
    static const int fontR[5][5] = { {1,1,1,1,0},{1,0,0,0,1},{1,1,1,1,0},{1,0,1,0,0},{1,0,0,1,1} };
    static const int fontP[5][5] = { {1,1,1,1,0},{1,0,0,0,1},{1,1,1,1,0},{1,0,0,0,0},{1,0,0,0,0} };
    static const int fontS[5][5] = { {0,1,1,1,1},{1,0,0,0,0},{0,1,1,1,0},{0,0,0,0,1},{1,1,1,1,0} };
    static const int fontI[5][5] = { {1,1,1,1,1},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{1,1,1,1,1} };
    static const int fontN[5][5] = { {1,0,0,0,1},{1,1,0,0,1},{1,0,1,0,1},{1,0,0,1,1},{1,0,0,0,1} };
    static const int fontC[5][5] = { {0,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{0,1,1,1,1} };
    static const int fontU[5][5] = { {1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0} };
    static const int fontD[5][5] = { {1,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,0} };
    static const int fontT[5][5] = { {1,1,1,1,1},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0} };
    static const int fontQ[5][5] = { {0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,1,0},{0,1,1,0,1} };
    static const int fontW[5][5] = { {1,0,0,0,1},{1,0,0,0,1},{1,0,1,0,1},{1,1,0,1,1},{1,0,0,0,1} };
    static const int fontL[5][5] = { {1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,1} };
    static const int fontH[5][5] = { {1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1} };
    static const int fontY[5][5] = { {1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0},{0,0,1,0,0},{0,0,1,0,0} };
    static const int fontB[5][5] = { {1,1,1,1,0},{1,0,0,0,1},{1,1,1,1,0},{1,0,0,0,1},{1,1,1,1,0} };
    static const int fontF[5][5] = { {1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,0},{1,0,0,0,0} };
    static const int fontK[5][5] = { {1,0,0,0,1},{1,0,0,1,0},{1,1,1,0,0},{1,0,0,1,0},{1,0,0,0,1} };
    static const int fontZ[5][5] = { {1,1,1,1,1},{0,0,0,1,0},{0,0,1,0,0},{0,1,0,0,0},{1,1,1,1,1} };
    static const int fontX[5][5] = { {1,0,0,0,1},{0,1,0,1,0},{0,0,1,0,0},{0,1,0,1,0},{1,0,0,0,1} };
    static const int fontJ[5][5] = { {0,0,0,0,1},{0,0,0,0,1},{0,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0} };

    static const int font0[5][5] = { {0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0} };
    static const int font1[5][5] = { {0,0,1,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,1,1,1,0} };
    static const int font2[5][5] = { {0,1,1,1,0},{1,0,0,0,1},{0,0,1,1,0},{0,1,0,0,0},{1,1,1,1,1} };
    static const int font3[5][5] = { {1,1,1,1,0},{0,0,0,0,1},{0,1,1,1,0},{0,0,0,0,1},{1,1,1,1,0} };
    static const int font4[5][5] = { {1,0,0,1,0},{1,0,0,1,0},{1,1,1,1,1},{0,0,0,1,0},{0,0,0,1,0} };
    static const int font5[5][5] = { {1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,0},{0,0,0,0,1},{1,1,1,1,0} };
    static const int font6[5][5] = { {0,1,1,1,0},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,1},{0,1,1,1,0} };
    static const int font7[5][5] = { {1,1,1,1,1},{0,0,0,0,1},{0,0,0,1,0},{0,0,1,0,0},{0,0,1,0,0} };
    static const int font8[5][5] = { {0,1,1,1,0},{1,0,0,0,1},{0,1,1,1,0},{1,0,0,0,1},{0,1,1,1,0} };
    static const int font9[5][5] = { {0,1,1,1,0},{1,0,0,0,1},{0,1,1,1,1},{0,0,0,0,1},{0,1,1,1,0} };

    static const int fontColon[5][5] = { {0,0,0,0,0},{0,0,1,0,0},{0,0,0,0,0},{0,0,1,0,0},{0,0,0,0,0} };
    static const int fontPipe[5][5] = { {0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0} };
    static const int fontMinus[5][5] = { {0,0,0,0,0},{0,0,0,0,0},{1,1,1,1,1},{0,0,0,0,0},{0,0,0,0,0} };
    static const int fontUnderscore[5][5] = { {0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{1,1,1,1,1} };

    const int (*grid)[5] = nullptr;
    if (ch == 'G') grid = fontG;
    else if (ch == 'A') grid = fontA;
    else if (ch == 'M') grid = fontM;
    else if (ch == 'E') grid = fontE;
    else if (ch == 'O') grid = fontO;
    else if (ch == 'V') grid = fontV;
    else if (ch == 'R') grid = fontR;
    else if (ch == 'P') grid = fontP;
    else if (ch == 'S') grid = fontS;
    else if (ch == 'I') grid = fontI;
    else if (ch == 'N') grid = fontN;
    else if (ch == 'C') grid = fontC;
    else if (ch == 'U') grid = fontU;
    else if (ch == 'D') grid = fontD;
    else if (ch == 'T') grid = fontT;
    else if (ch == 'Q') grid = fontQ;
    else if (ch == 'W') grid = fontW;
    else if (ch == 'L') grid = fontL;
    else if (ch == 'H') grid = fontH;
    else if (ch == 'Y') grid = fontY;
    else if (ch == 'B') grid = fontB;
    else if (ch == 'F') grid = fontF;
    else if (ch == 'K') grid = fontK;
    else if (ch == 'Z') grid = fontZ;
    else if (ch == 'X') grid = fontX;
    else if (ch == 'J') grid = fontJ;
    else if (ch == '0') grid = font0;
    else if (ch == '1') grid = font1;
    else if (ch == '2') grid = font2;
    else if (ch == '3') grid = font3;
    else if (ch == '4') grid = font4;
    else if (ch == '5') grid = font5;
    else if (ch == '6') grid = font6;
    else if (ch == '7') grid = font7;
    else if (ch == '8') grid = font8;
    else if (ch == '9') grid = font9;
    else if (ch == ':') grid = fontColon;
    else if (ch == '|') grid = fontPipe;
    else if (ch == '-') grid = fontMinus;
    else if (ch == '_') grid = fontUnderscore;

    if (!grid) return;

    HBRUSH brush = CreateSolidBrush(color);
    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 5; ++c) {
            if (grid[r][c]) {
                RECT pr = {
                    (long)(x + c * pSize),
                    (long)(y + r * pSize),
                    (long)(x + (c + 1) * pSize),
                    (long)(y + (r + 1) * pSize)
                };
                FillRect(hdc, &pr, brush);
            }
        }
    }
    DeleteObject(brush);
}
float Score;
void DrawPixelText(HDC hdc, const std::string& text, float centerX, float centerY, float pSize, COLORREF color) {
    float charWidth = 5 * pSize;
    float spacing = 1.5f * pSize;
    float totalWidth = 0;

    for (char c : text) {
        if (c == ' ') totalWidth += 3 * pSize + spacing;
        else totalWidth += charWidth + spacing;
    }
    if (!text.empty()) totalWidth -= spacing;

    float startX = centerX - totalWidth / 2.0f;
    float startY = centerY - (5 * pSize) / 2.0f;

    float curX = startX;
    for (char c : text) {
        if (c == ' ') {
            curX += 3 * pSize + spacing;
        }
        else {
            DrawPixelChar(hdc, c, curX, startY, pSize, color);
            curX += charWidth + spacing;
        }
    }
}

// ============================================================================
// [SCENE SYSTEM] 씬 인터페이스 및 관리자
// ============================================================================
class IGameScene {
public:
    virtual ~IGameScene() {}
    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
    virtual void Release() = 0;
    virtual void OnKeyDown(WPARAM wParam) {}
    virtual void OnLButtonDown(int mx, int my) {}
    virtual void OnMouseWheel(int zDelta) {}
    virtual float GetShakeTimer() const { return 0.0f; }
    virtual float GetShakeIntensity() const { return 0.0f; }
};

class SceneManager {
private:
    IGameScene* currentScene = nullptr;

public:
    static SceneManager& GetInstance() {
        static SceneManager instance;
        return instance;
    }

    void ChangeScene(IGameScene* newScene) {
        if (currentScene) {
            currentScene->Release();
            delete currentScene;
        }
        currentScene = newScene;
        if (currentScene) {
            currentScene->Init();
        }
    }

    void Update() {
        if (currentScene) currentScene->Update();
    }

    void Draw() {
        if (currentScene) currentScene->Draw();
    }

    void OnKeyDown(WPARAM wParam) {
        if (currentScene) currentScene->OnKeyDown(wParam);
    }

    void OnLButtonDown(int mx, int my) {
        if (currentScene) currentScene->OnLButtonDown(mx, my);
    }

    void OnMouseWheel(int zDelta) {
        if (currentScene) currentScene->OnMouseWheel(zDelta);
    }

    IGameScene* GetCurrentScene() const { return currentScene; }

    ~SceneManager() {
        if (currentScene) {
            currentScene->Release();
            delete currentScene;
        }
    }
};

class TitleScene;
class RankScene;
class PlayScene;

// ============================================================================
// [RANKING SCENE] 랭킹 확인 씬 선언
// ============================================================================
class RankScene : public IGameScene {
private:
    int scrollOffset = 0;

public:
    virtual void Init() override;
    virtual void Update() override;
    virtual void Release() override;
    virtual void Draw() override;
    virtual void OnKeyDown(WPARAM wParam) override;
    virtual void OnLButtonDown(int mx, int my) override;
    virtual void OnMouseWheel(int zDelta) override;
};

// ============================================================================
// [TITLE SCENE] 타이틀 씬 선언
// ============================================================================
class TitleScene : public IGameScene {
public:
    virtual void Init() override;
    virtual void Update() override;
    virtual void Release() override;
    virtual void Draw() override;
    virtual void OnKeyDown(WPARAM wParam) override;
    virtual void OnLButtonDown(int mx, int my) override;
};

// ============================================================================
// [PLAY SCENE] 메인 인게임 씬 선언
// ============================================================================
enum PlaySubState { PLAY_STATE_RUNNING, PLAY_STATE_PAUSED, PLAY_STATE_DYING, PLAY_STATE_GAMEOVER, PLAY_STATE_INPUT_INITIAL };

class PlayScene : public IGameScene {
private:
    PlaySubState state = PLAY_STATE_RUNNING;
    AABB arena = { 125.0f, 37.5f, 275.0f, 187.5f };
    Player player = { 200.0f, 112.5f, 3.0f, false, 0.0f, 0.0f };

    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<SmallPillar> pillars;

    std::vector<GiantPillar> giantPillars;

    StarItem starItem;
    SwordItem swordItem;
    ShieldItem shieldItem;
    std::vector<Particle> particles;

    bool hasShield = false;
    float shieldTimer = 0.0f;
    float invincibleTimer = 0.0f;

    float survivalTime = 0.0f;
    float worldSpeed = 1.0f;
    float ballSpawnTimer = 0.0f;
    float targetLockTimer = 0.0f;
    float giantPillarTimer = 0.0f;
    float smallPillarTimer = 0.0f;
    float starSpawnTimer = 0.0f;
    float swordSpawnTimer = 0.0f;
    float shieldSpawnTimer = 0.0f;
    float sightBonus = 0.0f;

    float shakeTimer = 0.0f;
    float shakeIntensity = 0.0f;

    float deathTimer = 0.0f;
    float deathStartX = 0.0f;
    float deathStartY = 0.0f;
    bool deathSplitSoundPlayed = false;

    std::string playerInitial = "AAA";
    bool rankSubmitted = false;

    const float DEATH_DISAPPEAR_TIME = 0.5f;
    const float DEATH_MOVE_TIME = 0.6f;
    const float DEATH_PAUSE_TIME = 0.75f;
    const float DEATH_SPLIT_TIME = 0.8f;

public:
    void TriggerShake(float intensity, float duration) {
        shakeIntensity = intensity;
        shakeTimer = duration;
    }

    void SpawnDashParticles(float x, float y) {
        Particle p;
        p.x = x; p.y = y;
        p.vx = 0.0f; p.vy = 0.0f;
        p.life = 0.25f;
        p.maxLife = 0.25f;
        p.color = RGB(0, 180, 240);
        p.isHeart = true;
        particles.push_back(p);
    }

    void SpawnPillarParticles(float x, float y, int count) {
        for (int i = 0; i < (int)(count * 1.5); ++i) {
            Particle p;
            p.x = x; p.y = y;
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float speed = 15.0f + (rand() % 40);
            p.vx = cosf(angle) * speed; p.vy = sinf(angle) * speed;
            p.life = 0.3f + (float)(rand() % 100) / 200.0f;
            p.maxLife = p.life;
            p.color = (rand() % 2 == 0) ? RGB(140, 90, 20) : RGB(80, 80, 100);
            p.isHeart = false;
            particles.push_back(p);
        }
    }

    void SpawnBulletHitParticles(float x, float y) {
        for (int i = 0; i < 5; ++i) {
            Particle p;
            p.x = x; p.y = y;
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float speed = 15.0f + (rand() % 35);
            p.vx = cosf(angle) * speed; p.vy = sinf(angle) * speed;
            p.life = 0.15f + (float)(rand() % 50) / 200.0f;
            p.maxLife = p.life;
            p.color = RGB(220, 60, 0);
            p.isHeart = false;
            particles.push_back(p);
        }
    }

    void SpawnEnemyDestroyParticles(float x, float y, EnemyType type, int count = 30) {
        COLORREF c1, c2;
        if (type == ENEMY_CIRCLE) { c1 = RGB(220, 30, 30); c2 = RGB(200, 100, 0); }
        else if (type == ENEMY_TRIANGLE) { c1 = RGB(180, 0, 150); c2 = RGB(120, 0, 180); }
        else { c1 = RGB(0, 150, 200); c2 = RGB(0, 180, 140); }

        for (int i = 0; i < count; ++i) {
            Particle p;
            p.x = x; p.y = y;
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float speed = 20.0f + (rand() % 60);
            p.vx = cosf(angle) * speed; p.vy = sinf(angle) * speed;
            p.life = 0.3f + (float)(rand() % 100) / 200.0f;
            p.maxLife = p.life;
            p.color = (rand() % 2 == 0) ? c1 : c2;
            p.isHeart = false;
            particles.push_back(p);
        }
    }

    void SpawnItemPickupParticles(float x, float y, int itemType) {
        COLORREF c1, c2;
        if (itemType == 0) { c1 = RGB(200, 140, 0); c2 = RGB(230, 180, 0); }
        else if (itemType == 1) { c1 = RGB(0, 140, 220); c2 = RGB(0, 180, 240); }
        else { c1 = RGB(20, 100, 230); c2 = RGB(80, 160, 255); }

        for (int i = 0; i < 25; ++i) {
            Particle p;
            p.x = x; p.y = y;
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float speed = 15.0f + (rand() % 45);
            p.vx = cosf(angle) * speed; p.vy = sinf(angle) * speed;
            p.life = 0.3f + (float)(rand() % 80) / 200.0f;
            p.maxLife = p.life;
            p.color = (rand() % 2 == 0) ? c1 : c2;
            p.isHeart = false;
            particles.push_back(p);
        }
    }

    void SpawnHexExplosionParticles(float x, float y) {
        for (int i = 0; i < 45; ++i) {
            Particle p;
            p.x = x; p.y = y;
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float speed = 40.0f + (rand() % 70);
            p.vx = cosf(angle) * speed; p.vy = sinf(angle) * speed;
            p.life = 0.35f + (float)(rand() % 100) / 200.0f;
            p.maxLife = p.life;
            p.color = (rand() % 3 == 0) ? RGB(220, 30, 30) : ((rand() % 2 == 0) ? RGB(220, 120, 0) : RGB(180, 160, 0));
            p.isHeart = false;
            particles.push_back(p);
        }
    }

    void BuildShapePolygon(GiantPillar& gp) {
        gp.shapeVertices.clear();
        float cx = gp.shapeCenterX;
        float cy = gp.shapeCenterY;

        if (gp.shapeType == 0) {
            int pointsCount = 10;
            float rOut = 45.0f;
            float rIn = 22.0f;
            for (int i = 0; i < pointsCount; ++i) {
                float angle = i * 3.14159265f / 5.0f - 3.14159265f / 2.0f;
                float r = (i % 2 == 0) ? rOut : rIn;
                float px = cx + cosf(angle) * r;
                float py = cy + sinf(angle) * r;
                gp.shapeVertices.push_back({ px, py });
            }
        }
        else if (gp.shapeType == 1) {
            int numPoints = 30;
            for (int i = 0; i < numPoints; ++i) {
                float t = (float)i / numPoints * 2.0f * 3.14159265f;
                float x = 16.0f * powf(sinf(t), 3.0f);
                float y = -(13.0f * cosf(t) - 5.0f * cosf(2.0f * t) - 2.0f * cosf(3.0f * t) - cosf(4.0f * t));
                float px = cx + x * 2.5f;
                float py = cy + y * 2.5f - 5.0f;
                gp.shapeVertices.push_back({ px, py });
            }
        }
        else {
            int numPoints = 30;
            for (int i = 0; i < numPoints; ++i) {
                float t = (float)i / numPoints * 2.0f * 3.14159265f;
                float r = 40.0f;
                float px = cx + cosf(t) * r * (1.0f - sinf(t) * 0.3f);
                float py = cy + sinf(t) * r;
                gp.shapeVertices.push_back({ px, py });
            }
        }
    }

    bool TryKillPlayer() {
        if (invincibleTimer > 0.0f) return false;

        if (hasShield) {
            hasShield = false;
            shieldTimer = 0.0f;
            invincibleTimer = 1.2f;
            SoundManager::PlayRetroBeep(115, 180);
            TriggerShake(3.0f, 0.2f);
            SpawnItemPickupParticles(player.x, player.y, 2);
            return false;
        }

        if (state == PLAY_STATE_RUNNING) {
            state = PLAY_STATE_DYING;
            deathTimer = 0.0f;
            deathStartX = player.x;
            deathStartY = player.y;
            deathSplitSoundPlayed = false;

            SoundManager::PlayRetroBeep(36, 400);
            TriggerShake(3.0f, 0.25f);
        }
        return true;
    }

    Enemy SpawnEnemyOutsideArena(EnemyType type) {
        Enemy e;
        e.type = type;
        e.radius = 6.0f;
        e.hasEntered = false;
        e.hexPauseTimer = 3.0f + (float)(rand() % 30) / 10.0f;

        float centerX = (arena.minX + arena.maxX) / 2.0f;
        float centerY = (arena.minY + arena.maxY) / 2.0f;

        int side = rand() % 4;
        float spawnOffset = 60.0f;
        if (side == 0) {
            e.x = arena.minX + (rand() % (int)(arena.maxX - arena.minX));
            e.y = arena.minY - spawnOffset;
        }
        else if (side == 1) {
            e.x = arena.minX + (rand() % (int)(arena.maxX - arena.minX));
            e.y = arena.maxY + spawnOffset;
        }
        else if (side == 2) {
            e.x = arena.minX - spawnOffset;
            e.y = arena.minY + (rand() % (int)(arena.maxY - arena.minY));
        }
        else {
            e.x = arena.maxX + spawnOffset;
            e.y = arena.minY + (rand() % (int)(arena.maxY - arena.minY));
        }

        float targetX = centerX + (rand() % 60 - 30);
        float targetY = centerY + (rand() % 60 - 30);
        float dx = targetX - e.x;
        float dy = targetY - e.y;
        float len = std::sqrt(dx * dx + dy * dy);
        float speed = 32.5f;
        if (len > 0.001f) {
            e.vx = (dx / len) * speed;
            e.vy = (dy / len) * speed;
        }
        else {
            e.vx = speed; e.vy = 0.0f;
        }
        e.angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        return e;
    }

    bool CheckPlayerVsCircle(const Player& p, float cx, float cy, float cradius) {
        float dx = p.x - cx;
        float dy = p.y - cy;
        float distSq = dx * dx + dy * dy;
        float r = p.radius + cradius;
        return distSq < (r * r);
    }

    bool HandleEnemyVsAABB(Enemy& enemy, const AABB& box) {
        float closestX = (std::max)(box.minX, (std::min)(enemy.x, box.maxX));
        float closestY = (std::max)(box.minY, (std::min)(enemy.y, box.maxY));

        float diffX = enemy.x - closestX;
        float diffY = enemy.y - closestY;
        float distSq = (diffX * diffX) + (diffY * diffY);

        if (distSq < (enemy.radius * enemy.radius)) {
            if (std::abs(diffX) > std::abs(diffY)) {
                enemy.vx = (diffX > 0) ? std::abs(enemy.vx) : -std::abs(enemy.vx);
                enemy.x += (diffX > 0 ? 1.0f : -1.0f);
            }
            else {
                enemy.vy = (diffY > 0) ? std::abs(enemy.vy) : -std::abs(enemy.vy);
                enemy.y += (diffY > 0 ? 1.0f : -1.0f);
            }
            return true;
        }
        return false;
    }

    void ResolvePlayerWallCollisions() {
        float centerX = (arena.minX + arena.maxX) / 2.0f;
        float centerY = (arena.minY + arena.maxY) / 2.0f;

        float borderMargin = 2.0f;
        player.x = (std::max)(arena.minX + borderMargin + player.radius, (std::min)(player.x, arena.maxX - borderMargin - player.radius));
        player.y = (std::max)(arena.minY + borderMargin + player.radius, (std::min)(player.y, arena.maxY - borderMargin - player.radius));

        for (auto& pillar : pillars) {
            if (!pillar.isActive) continue;
            if (player.x >= pillar.box.minX && player.x <= pillar.box.maxX &&
                player.y >= pillar.box.minY && player.y <= pillar.box.maxY) {
                float dl = player.x - pillar.box.minX;
                float dr = pillar.box.maxX - player.x;
                float dt_edge = player.y - pillar.box.minY;
                float db = pillar.box.maxY - player.y;

                float minD = dl;
                int side = 0;
                if (dr < minD) { minD = dr; side = 1; }
                if (dt_edge < minD) { minD = dt_edge; side = 2; }
                if (db < minD) { minD = db; side = 3; }

                if (side == 0) player.x = pillar.box.minX - player.radius;
                else if (side == 1) player.x = pillar.box.maxX + player.radius;
                else if (side == 2) player.y = pillar.box.minY - player.radius;
                else if (side == 3) player.y = pillar.box.maxY + player.radius;
            }
            else {
                float closestX = (std::max)(pillar.box.minX, (std::min)(player.x, pillar.box.maxX));
                float closestY = (std::max)(pillar.box.minY, (std::min)(player.y, pillar.box.maxY));
                float diffX = player.x - closestX;
                float diffY = player.y - closestY;
                float distSq = (diffX * diffX) + (diffY * diffY);

                if (distSq < (player.radius * player.radius) && distSq > 0.0001f) {
                    float dist = std::sqrt(distSq);
                    float overlap = player.radius - dist;
                    overlap = (std::min)(overlap, 10.0f);
                    player.x += (diffX / dist) * overlap;
                    player.y += (diffY / dist) * overlap;
                }
            }
        }

        for (const auto& gp : giantPillars) {
            if (gp.state != GIANT_SOLID) continue;

            if (gp.type == PILLAR_STRAIGHT) {
                float px = player.x - centerX;
                float py = player.y - centerY;
                float perpDist = px * gp.normX + py * gp.normY;
                float paraDist = px * gp.dirX + py * gp.dirY;

                if (std::abs(perpDist) < (player.radius + gp.thickness / 2.0f) && std::abs(paraDist) <= 250.0f) {
                    float sign = (perpDist >= 0) ? 1.0f : -1.0f;
                    float overlap = (player.radius + gp.thickness / 2.0f) - std::abs(perpDist);
                    overlap = (std::min)(overlap, 10.0f);
                    player.x += gp.normX * sign * overlap;
                    player.y += gp.normY * sign * overlap;
                }
            }
            else if (gp.type == PILLAR_CURVED) {
                int numSegs = 20;
                float minDist = 99999.0f;
                float bestProjX = 0.0f, bestProjY = 0.0f;

                for (int i = 0; i < numSegs; ++i) {
                    float tA = (float)i / numSegs;
                    float tB = (float)(i + 1) / numSegs;
                    float ax, ay, bx, by, projX, projY;
                    GetBezierPoint(tA, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, ax, ay);
                    GetBezierPoint(tB, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, bx, by);

                    float dist = DistToSegment(player.x, player.y, ax, ay, bx, by, projX, projY);
                    if (dist < minDist) {
                        minDist = dist; bestProjX = projX; bestProjY = projY;
                    }
                }

                if (minDist < (player.radius + gp.thickness / 2.0f)) {
                    float diffX = player.x - bestProjX;
                    float diffY = player.y - bestProjY;
                    float dLen = std::sqrt(diffX * diffX + diffY * diffY);
                    if (dLen > 0.001f) {
                        float overlap = (player.radius + gp.thickness / 2.0f) - dLen;
                        overlap = (std::min)(overlap, 10.0f);
                        player.x += (diffX / dLen) * overlap;
                        player.y += (diffY / dLen) * overlap;
                    }
                }
            }
            else if (gp.type == PILLAR_SHAPE) {
                float scaleFactor = 1.0f;
                int numVerts = (int)gp.shapeVertices.size();
                float minDist = 99999.0f;
                float bestProjX = 0.0f, bestProjY = 0.0f;

                for (int i = 0; i < numVerts; ++i) {
                    float ax = gp.shapeCenterX + (gp.shapeVertices[i].first - gp.shapeCenterX) * scaleFactor;
                    float ay = gp.shapeCenterY + (gp.shapeVertices[i].second - gp.shapeCenterY) * scaleFactor;
                    float bx = gp.shapeCenterX + (gp.shapeVertices[(i + 1) % numVerts].first - gp.shapeCenterX) * scaleFactor;
                    float by = gp.shapeCenterY + (gp.shapeVertices[(i + 1) % numVerts].second - gp.shapeCenterY) * scaleFactor;

                    float projX, projY;
                    float dist = DistToSegment(player.x, player.y, ax, ay, bx, by, projX, projY);
                    if (dist < minDist) {
                        minDist = dist; bestProjX = projX; bestProjY = projY;
                    }
                }

                if (minDist < (player.radius + gp.thickness / 2.0f)) {
                    float diffX = player.x - bestProjX;
                    float diffY = player.y - bestProjY;
                    float dLen = std::sqrt(diffX * diffX + diffY * diffY);
                    if (dLen > 0.001f) {
                        float overlap = (player.radius + gp.thickness / 2.0f) - dLen;
                        overlap = (std::min)(overlap, 10.0f);
                        player.x += (diffX / dLen) * overlap;
                        player.y += (diffY / dLen) * overlap;
                    }
                }
            }
        }
    }

    virtual void Init() override;
    virtual void Update() override;
    virtual void Draw() override;
    virtual void Release() override;
    virtual void OnKeyDown(WPARAM wParam) override;
    virtual void OnLButtonDown(int mx, int my) override;
    virtual float GetShakeTimer() const override { return shakeTimer; }
    virtual float GetShakeIntensity() const override { return shakeIntensity; }
};

// ============================================================================
// [SCENE METHOD DEFINITIONS]
// ============================================================================
void RankScene::Init() { scrollOffset = 0; }
void RankScene::Update() {}
void RankScene::Release() {}

void RankScene::Draw() {
    HDC hdc = g_RenderDC;
    float CX = VIRTUAL_WIDTH / 2.0f;

    DrawPixelText(hdc, "RANKINGS", CX, 25, 2.0f, RGB(20, 20, 30));

    RECT listArea = { 80, 50, 320, 190 };
    HBRUSH boxBg = CreateSolidBrush(RGB(235, 235, 242));
    FillRect(hdc, &listArea, boxBg);
    DeleteObject(boxBg);
    DrawPixelRectBorder(hdc, 80, 50, 320, 190, 1.0f, RGB(100, 100, 120));

    HRGN rgn = CreateRectRgn(81, 51, 320, 190);
    SelectClipRgn(hdc, rgn);

    int startY = 60 - scrollOffset;
    if (g_RankList.empty()) {
        DrawPixelText(hdc, "NO RECORDS YET", CX, startY + 20, 1.0f, RGB(100, 100, 100));
    }
    else {
        for (size_t i = 0; i < g_RankList.size(); ++i) {
            int y = startY + (int)i * 22;
            if (y > 35 && y < 210) {
                std::string rankStr = std::to_string(i + 1) + ". " + std::string(g_RankList[i].name) + " - " + std::to_string(g_RankList[i].score) + "S";
                DrawPixelText(hdc, rankStr, CX, (float)y, 1.0f, RGB(40, 40, 50));
            }
        }
    }

    SelectClipRgn(hdc, NULL);
    DeleteObject(rgn);

    RECT backBtn = { 150, 200, 250, 220 };
    bool hover = (g_MouseX >= backBtn.left && g_MouseX <= backBtn.right && g_MouseY >= backBtn.top && g_MouseY <= backBtn.bottom);
    HBRUSH bBrush = CreateSolidBrush(hover ? RGB(220, 40, 40) : RGB(220, 220, 230));
    FillRect(hdc, &backBtn, bBrush); DeleteObject(bBrush);
    DrawPixelText(hdc, "BACK", CX, 210, 1.1f, hover ? RGB(255, 255, 255) : RGB(20, 20, 30));
}

void RankScene::OnKeyDown(WPARAM wParam) {
    if (wParam == VK_ESCAPE || wParam == VK_SPACE) {
        SceneManager::GetInstance().ChangeScene((IGameScene*)new TitleScene());
        SoundManager::PlayRetroBeep(70, 80);
    }
    else if (wParam == VK_UP) {
        scrollOffset = (std::max)(0, scrollOffset - 20);
    }
    else if (wParam == VK_DOWN) {
        int maxScroll = (int)(g_RankList.size() * 22) - 120;
        if (maxScroll < 0) maxScroll = 0;
        scrollOffset = (std::min)(maxScroll, scrollOffset + 20);
    }
}

void RankScene::OnLButtonDown(int mx, int my) {
    if (mx >= 150 && mx <= 250 && my >= 200 && my <= 220) {
        SceneManager::GetInstance().ChangeScene((IGameScene*)new TitleScene());
        SoundManager::PlayRetroBeep(70, 80);
    }
}

void RankScene::OnMouseWheel(int zDelta) {
    if (zDelta > 0) {
        scrollOffset = (std::max)(0, scrollOffset - 25);
    }
    else {
        int maxScroll = (int)(g_RankList.size() * 22) - 120;
        if (maxScroll < 0) maxScroll = 0;
        scrollOffset = (std::min)(maxScroll, scrollOffset + 25);
    }
}

void TitleScene::Init() {}
void TitleScene::Update() {}
void TitleScene::Release() {}

void TitleScene::Draw() {
    HDC hdc = g_RenderDC; float CX = VIRTUAL_WIDTH / 2.0f; float CY = VIRTUAL_HEIGHT / 2.0f;
    DrawPixelText(hdc, "SURVIVAL", CX, CY - 60, 2.2f, RGB(20, 20, 30));

    RECT startBtn = { 125, (long)(CY + 10), 275, (long)(CY + 35) };
    RECT quitBtn = { 125, (long)(CY + 45), 275, (long)(CY + 70) };

    bool hStart = (g_MouseX >= startBtn.left && g_MouseX <= startBtn.right && g_MouseY >= startBtn.top && g_MouseY <= startBtn.bottom);
    bool hQuit = (g_MouseX >= quitBtn.left && g_MouseX <= quitBtn.right && g_MouseY >= quitBtn.top && g_MouseY <= quitBtn.bottom);

    HBRUSH brStart = CreateSolidBrush(hStart ? RGB(0, 140, 220) : RGB(220, 220, 230));
    HBRUSH brQuit = CreateSolidBrush(hQuit ? RGB(220, 40, 40) : RGB(220, 220, 230));

    FillRect(hdc, &startBtn, brStart); DeleteObject(brStart);
    FillRect(hdc, &quitBtn, brQuit); DeleteObject(brQuit);

    DrawPixelText(hdc, "START GAME", CX, CY + 22.5f, 1.2f, hStart ? RGB(255, 255, 255) : RGB(20, 20, 30));
    DrawPixelText(hdc, "END RELAY", CX, CY + 57.5f, 1.2f, hQuit ? RGB(255, 255, 255) : RGB(20, 20, 30));

    // 조작키 안내
    DrawPixelText(hdc, "[WASD] MOVE  [SPACE] DASH", CX, CY + 100.0f, 1.2f, RGB(100, 100, 100));
}

void TitleScene::OnKeyDown(WPARAM wParam) {
    if (wParam == VK_SPACE) {
        float CY = VIRTUAL_HEIGHT / 2.0f;
        // 마우스가 END RELAY 위에 올려져 있을 때 스페이스를 누르면 릴레이 종료
        if (g_MouseX >= 100 && g_MouseX <= 300 && g_MouseY >= CY + 50 && g_MouseY <= CY + 80) {
            isForceEndRelay = true;
            gameoverchecker = true;
            SoundManager::StopBGM();
            SoundManager::PlayFanfare_2();
        }
        else {
            // 그 외에는 정상적으로 게임 A 플레이 시작!
            SceneManager::GetInstance().ChangeScene((IGameScene*)new PlayScene());
            SoundManager::PlayRetroBeep(90, 80);
        }
    }
}
void TitleScene::OnLButtonDown(int mx, int my) {
    float CY = VIRTUAL_HEIGHT / 2.0f;
    if (mx >= 125 && mx <= 275 && my >= CY + 10 && my <= CY + 35) { SceneManager::GetInstance().ChangeScene((IGameScene*)new PlayScene()); SoundManager::PlayRetroBeep(90, 80); }
    else if (mx >= 125 && mx <= 275 && my >= CY + 45 && my <= CY + 70) { isForceEndRelay = true; gameoverchecker = true; } // END RELAY
}

void PlayScene::Init() {
    player.x = 200.0f;
    player.y = 112.5f;
    player.radius = 3.0f;
    player.isDashing = false;
    player.dashDuration = 0.0f;
    player.dashCooldown = 0.0f;

    hasShield = false;
    shieldTimer = 0.0f;
    invincibleTimer = 0.0f;

    enemies.clear();
    bullets.clear();
    pillars.clear();
    giantPillars.clear();
    particles.clear();

    enemies.push_back(SpawnEnemyOutsideArena(ENEMY_CIRCLE));

    starItem.active = false;
    swordItem.active = false;
    shieldItem.active = false;
    sightBonus = 0.0f;

    survivalTime = 0.0f;
    worldSpeed = 1.0f;
    ballSpawnTimer = 8.0f;
    targetLockTimer = 12.0f;
    giantPillarTimer = 6.0f;
    smallPillarTimer = 10.0f;
    starSpawnTimer = 12.0f;
    swordSpawnTimer = 15.0f;
    shieldSpawnTimer = 18.0f;
    shakeTimer = 0.0f;
    deathTimer = 0.0f;
    deathSplitSoundPlayed = false;
    rankSubmitted = false;
    playerInitial = "AAA";
    state = PLAY_STATE_RUNNING;
}

void PlayScene::Update() {
    float dt = g_DeltaTime;

    if (state == PLAY_STATE_PAUSED || state == PLAY_STATE_GAMEOVER || state == PLAY_STATE_INPUT_INITIAL) return;

    if (state == PLAY_STATE_DYING) {
        deathTimer += dt;
        if (shakeTimer > 0.0f) shakeTimer -= dt;

        float centerX = (arena.minX + arena.maxX) / 2.0f;
        float centerY = (arena.minY + arena.maxY) / 2.0f;

        if (deathTimer <= DEATH_DISAPPEAR_TIME) {}
        else if (deathTimer <= DEATH_DISAPPEAR_TIME + DEATH_MOVE_TIME) {
            float t = (deathTimer - DEATH_DISAPPEAR_TIME) / DEATH_MOVE_TIME;
            float easeT = sinf(t * 3.14159265f * 0.5f);
            player.x = deathStartX + (centerX - deathStartX) * easeT;
            player.y = deathStartY + (centerY - deathStartY) * easeT;
        }
        else if (deathTimer <= DEATH_DISAPPEAR_TIME + DEATH_MOVE_TIME + DEATH_PAUSE_TIME) {
            player.x = centerX; player.y = centerY;
        }
        else {
            player.x = centerX; player.y = centerY;
            if (!deathSplitSoundPlayed) {
                deathSplitSoundPlayed = true;
                SoundManager::PlayRetroBeep(28, 500);
                TriggerShake(5.0f, 0.3f);
            }
        }

        if (deathTimer >= (DEATH_DISAPPEAR_TIME + DEATH_MOVE_TIME + DEATH_PAUSE_TIME + DEATH_SPLIT_TIME)) {
            state = PLAY_STATE_INPUT_INITIAL;
            gameoverchecker = true;              // 👈 이걸로 교체! (릴레이로 넘김)
            Score = survivalTime;
        }
        return;
    }

    survivalTime += dt;

    if (shakeTimer > 0.0f) shakeTimer -= dt;
    for (auto& p : particles) {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.life <= 0.0f; }), particles.end());

    worldSpeed = 1.0f + (survivalTime * 0.012f);
    float effectiveDt = dt * worldSpeed;

    if (sightBonus > 0.0f) {
        sightBonus -= 15.0f * dt;
        if (sightBonus < 0.0f) sightBonus = 0.0f;
    }

    if (invincibleTimer > 0.0f) invincibleTimer -= dt;

    if (hasShield) {
        shieldTimer -= dt;
        if (shieldTimer <= 0.0f) hasShield = false;
    }

    if (player.dashDuration > 0.0f) {
        player.dashDuration -= dt;
        if (player.dashDuration <= 0.0f) player.isDashing = false;
        SpawnDashParticles(player.x, player.y);
    }
    if (player.dashCooldown > 0.0f) player.dashCooldown -= dt;

    float totalMoveSpeed = player.isDashing ? 275.0f : 110.0f;
    int subSteps = player.isDashing ? 4 : 1;
    float subDt = dt / subSteps;
    float subSpeed = totalMoveSpeed * subDt;

    for (int step = 0; step < subSteps; ++step) {
        float dx = 0.0f, dy = 0.0f;
        if (GetAsyncKeyState('W') & 0x8000) dy -= subSpeed;
        if (GetAsyncKeyState('S') & 0x8000) dy += subSpeed;
        if (GetAsyncKeyState('A') & 0x8000) dx -= subSpeed;
        if (GetAsyncKeyState('D') & 0x8000) dx += subSpeed;

        player.x += dx;
        ResolvePlayerWallCollisions();

        player.y += dy;
        ResolvePlayerWallCollisions();
    }

    float centerX = (arena.minX + arena.maxX) / 2.0f;
    float centerY = (arena.minY + arena.maxY) / 2.0f;

    giantPillarTimer -= dt;
    if (giantPillarTimer <= 0.0f && giantPillars.empty()) {
        int patternType = rand() % 3;

        if (patternType == 0) {
            GiantPillar gp;
            gp.state = GIANT_WARNING;
            gp.timer = 1.5f;
            gp.type = (rand() % 2 == 0) ? PILLAR_STRAIGHT : PILLAR_CURVED;

            if (gp.type == PILLAR_STRAIGHT) {
                gp.angle = (float)(rand() % 360) * 3.14159265f / 180.0f;
                gp.dirX = cosf(gp.angle); gp.dirY = sinf(gp.angle);
                gp.normX = -sinf(gp.angle); gp.normY = cosf(gp.angle);
            }
            else {
                float angle0 = (float)(rand() % 360) * 3.14159265f / 180.0f;
                float deltaAngle = (120.0f + (rand() % 120)) * 3.14159265f / 180.0f;
                float angle2 = angle0 + deltaAngle;
                float R = 275.0f;
                gp.p0x = centerX + cosf(angle0) * R; gp.p0y = centerY + sinf(angle0) * R;
                gp.p2x = centerX + cosf(angle2) * R; gp.p2y = centerY + sinf(angle2) * R;
                float midX = 0.5f * (gp.p0x + gp.p2x);
                float midY = 0.5f * (gp.p0y + gp.p2y);
                gp.p1x = 2.0f * centerX - midX + (rand() % 30 - 15);
                gp.p1y = 2.0f * centerY - midY + (rand() % 30 - 15);
            }
            giantPillars.push_back(gp);
        }
        else if (patternType == 1) {
            for (int i = 0; i < 2; ++i) {
                GiantPillar gp;
                gp.state = GIANT_WARNING;
                gp.timer = 1.5f;
                gp.type = PILLAR_STRAIGHT;
                gp.angle = (float)(i * 90 + (rand() % 45)) * 3.14159265f / 180.0f;
                gp.dirX = cosf(gp.angle); gp.dirY = sinf(gp.angle);
                gp.normX = -sinf(gp.angle); gp.normY = cosf(gp.angle);
                giantPillars.push_back(gp);
            }
        }
        else {
            GiantPillar gp;
            gp.state = GIANT_WARNING;
            gp.timer = 1.5f;
            gp.type = PILLAR_SHAPE;
            gp.shapeType = rand() % 3;
            gp.shapeCenterX = arena.minX + 40.0f + (rand() % (int)(arena.maxX - arena.minX - 80.0f));
            gp.shapeCenterY = arena.minY + 40.0f + (rand() % (int)(arena.maxY - arena.minY - 80.0f));
            BuildShapePolygon(gp);
            giantPillars.push_back(gp);
        }
    }

    for (auto it = giantPillars.begin(); it != giantPillars.end();) {
        GiantPillar& gp = *it;

        if (gp.state == GIANT_WARNING) {
            gp.timer -= dt;
            if (gp.timer <= 0.0f) {
                gp.state = GIANT_EXTENDING;
                gp.timer = 0.35f;
                gp.currentLength = -250.0f;
                SoundManager::PlayRetroBeep(40, 250);
                TriggerShake(4.0f, 0.35f);
            }
        }
        else if (gp.state == GIANT_EXTENDING || gp.state == GIANT_SOLID) {
            if (gp.state == GIANT_EXTENDING) {
                gp.timer -= dt;
                float progress = 1.0f - (gp.timer / 0.35f);
                if (progress > 1.0f) progress = 1.0f;
                gp.currentLength = -250.0f + (progress * 500.0f);

                if (gp.timer <= 0.0f) {
                    gp.state = GIANT_SOLID;
                    gp.timer = 5.5f;
                    TriggerShake(7.0f, 0.3f);
                    SpawnPillarParticles(centerX, centerY, 20);
                    SoundManager::PlayRetroBeep(32, 300);
                }
            }
            else {
                gp.timer -= dt;
                if (gp.timer <= 0.0f) {
                    it = giantPillars.erase(it);
                    if (giantPillars.empty()) {
                        giantPillarTimer = 7.0f + (rand() % 40) / 10.0f;
                    }
                    continue;
                }
            }

            if (gp.type == PILLAR_STRAIGHT) {
                float px = player.x - centerX;
                float py = player.y - centerY;
                float perpDist = px * gp.normX + py * gp.normY;
                float paraDist = px * gp.dirX + py * gp.dirY;

                if (gp.state == GIANT_EXTENDING) {
                    if (std::abs(perpDist) < (player.radius + gp.thickness / 2.0f) &&
                        paraDist <= gp.currentLength && paraDist >= -250.0f) {
                        TryKillPlayer();
                    }
                }
            }
            else if (gp.type == PILLAR_CURVED) {
                int numSegs = 20;
                float maxT = (gp.state == GIANT_SOLID) ? 1.0f : (1.0f - gp.timer / 0.35f);
                maxT = (std::max)(0.05f, (std::min)(1.0f, maxT));

                float minDist = 99999.0f;
                for (int i = 0; i < numSegs; ++i) {
                    float tA = ((float)i / numSegs) * maxT;
                    float tB = ((float)(i + 1) / numSegs) * maxT;
                    float ax, ay, bx, by, projX, projY;
                    GetBezierPoint(tA, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, ax, ay);
                    GetBezierPoint(tB, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, bx, by);

                    float dist = DistToSegment(player.x, player.y, ax, ay, bx, by, projX, projY);
                    if (dist < minDist) {
                        minDist = dist;
                    }
                }

                if (minDist < (player.radius + gp.thickness / 2.0f)) {
                    if (gp.state == GIANT_EXTENDING) {
                        TryKillPlayer();
                    }
                }
            }
            else if (gp.type == PILLAR_SHAPE) {
                float scaleFactor = (gp.state == GIANT_SOLID) ? 1.0f : (1.0f - gp.timer / 0.35f);
                scaleFactor = (std::max)(0.05f, (std::min)(1.0f, scaleFactor));

                int numVerts = (int)gp.shapeVertices.size();
                for (int i = 0; i < numVerts; ++i) {
                    float ax = gp.shapeCenterX + (gp.shapeVertices[i].first - gp.shapeCenterX) * scaleFactor;
                    float ay = gp.shapeCenterY + (gp.shapeVertices[i].second - gp.shapeCenterY) * scaleFactor;
                    float bx = gp.shapeCenterX + (gp.shapeVertices[(i + 1) % numVerts].first - gp.shapeCenterX) * scaleFactor;
                    float by = gp.shapeCenterY + (gp.shapeVertices[(i + 1) % numVerts].second - gp.shapeCenterY) * scaleFactor;

                    float projX, projY;
                    float dist = DistToSegment(player.x, player.y, ax, ay, bx, by, projX, projY);
                    if (dist < (player.radius + gp.thickness / 2.0f)) {
                        if (gp.state == GIANT_EXTENDING) {
                            TryKillPlayer();
                        }
                    }
                }
            }
        }
        ++it;
    }

    smallPillarTimer -= dt;
    if (smallPillarTimer <= 0.0f) {
        SmallPillar sp;
        sp.isWarning = true; sp.isActive = false; sp.timer = 1.0f; sp.hp = 4;

        bool validPosition = false;
        float rx = 0.0f, ry = 0.0f;

        for (int attempt = 0; attempt < 20; ++attempt) {
            rx = arena.minX + 20 + (rand() % (int)(arena.maxX - arena.minX - 40));
            ry = arena.minY + 20 + (rand() % (int)(arena.maxY - arena.minY - 40));
            AABB candBox = { rx - 10, ry - 10, rx + 10, ry + 10 };

            bool overlap = false;
            for (const auto& existing : pillars) {
                if (!(candBox.maxX < existing.box.minX || candBox.minX > existing.box.maxX ||
                    candBox.maxY < existing.box.minY || candBox.minY > existing.box.maxY)) {
                    overlap = true; break;
                }
            }
            if (!overlap) { validPosition = true; break; }
        }

        if (validPosition) {
            sp.box = { rx - 8, ry - 8, rx + 8, ry + 8 };
            pillars.push_back(sp);
        }
        smallPillarTimer = 11.0f;
    }

    for (auto& pillar : pillars) {
        if (pillar.isWarning) {
            pillar.timer -= dt;
            if (pillar.timer <= 0.0f) {
                pillar.isWarning = false; pillar.isActive = true;
                SoundManager::PlayRetroBeep(52, 100); TriggerShake(3.0f, 0.2f);
                float cx = (pillar.box.minX + pillar.box.maxX) / 2.0f;
                float cy = (pillar.box.minY + pillar.box.maxY) / 2.0f;
                SpawnPillarParticles(cx, cy, 8);
            }
        }
    }

    float magnetRadius = 39.0f;

    if (!starItem.active) {
        starSpawnTimer -= dt;
        if (starSpawnTimer <= 0.0f) {
            bool validStar = false;
            float sx = 0.0f, sy = 0.0f;

            for (int attempt = 0; attempt < 30; ++attempt) {
                sx = arena.minX + 15.0f + (rand() % (int)(arena.maxX - arena.minX - 30.0f));
                sy = arena.minY + 15.0f + (rand() % (int)(arena.maxY - arena.minY - 30.0f));

                bool overlap = false;
                for (const auto& pillar : pillars) {
                    if (sx >= pillar.box.minX - 12.0f && sx <= pillar.box.maxX + 12.0f &&
                        sy >= pillar.box.minY - 12.0f && sy <= pillar.box.maxY + 12.0f) {
                        overlap = true; break;
                    }
                }
                if (!overlap) { validStar = true; break; }
            }

            if (validStar) {
                starItem.x = sx; starItem.y = sy;
                starItem.active = true; starItem.life = 10.0f; starItem.angle = 0.0f;
            }
            starSpawnTimer = 18.0f;
        }
    }
    else {
        starItem.angle += 2.0f * dt; starItem.life -= dt;
        float dx = player.x - starItem.x; float dy = player.y - starItem.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < magnetRadius && dist > 0.001f) {
            float pullSpeed = 160.0f * (1.0f - (dist / magnetRadius) * 0.4f);
            starItem.x += (dx / dist) * pullSpeed * dt;
            starItem.y += (dy / dist) * pullSpeed * dt;
        }

        if (starItem.life <= 0.0f) starItem.active = false;
        else if (CheckPlayerVsCircle(player, starItem.x, starItem.y, starItem.radius)) {
            starItem.active = false; sightBonus = 150.0f;
            SoundManager::PlayRetroBeep(100, 150); TriggerShake(2.0f, 0.1f);
            SpawnItemPickupParticles(starItem.x, starItem.y, 0);
        }
    }

    if (!swordItem.active) {
        swordSpawnTimer -= dt;
        if (swordSpawnTimer <= 0.0f) {
            bool validSword = false;
            float sx = 0.0f, sy = 0.0f;

            for (int attempt = 0; attempt < 30; ++attempt) {
                sx = arena.minX + 15.0f + (rand() % (int)(arena.maxX - arena.minX - 30.0f));
                sy = arena.minY + 15.0f + (rand() % (int)(arena.maxY - arena.minY - 30.0f));

                bool overlap = false;
                for (const auto& pillar : pillars) {
                    if (sx >= pillar.box.minX - 12.0f && sx <= pillar.box.maxX + 12.0f &&
                        sy >= pillar.box.minY - 12.0f && sy <= pillar.box.maxY + 12.0f) {
                        overlap = true; break;
                    }
                }
                if (!overlap) { validSword = true; break; }
            }

            if (validSword) {
                swordItem.x = sx; swordItem.y = sy;
                swordItem.active = true; swordItem.life = 10.0f; swordItem.angle = 0.0f;
            }
            swordSpawnTimer = 22.0f;
        }
    }
    else {
        swordItem.angle += 3.0f * dt; swordItem.life -= dt;
        float dx = player.x - swordItem.x; float dy = player.y - swordItem.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < magnetRadius && dist > 0.001f) {
            float pullSpeed = 160.0f * (1.0f - (dist / magnetRadius) * 0.4f);
            swordItem.x += (dx / dist) * pullSpeed * dt;
            swordItem.y += (dy / dist) * pullSpeed * dt;
        }

        if (swordItem.life <= 0.0f) swordItem.active = false;
        else if (CheckPlayerVsCircle(player, swordItem.x, swordItem.y, swordItem.radius)) {
            swordItem.active = false;
            SoundManager::PlayRetroBeep(110, 150); TriggerShake(3.0f, 0.15f);
            SpawnItemPickupParticles(swordItem.x, swordItem.y, 1);

            if (!enemies.empty()) {
                int idx = rand() % enemies.size();
                SpawnEnemyDestroyParticles(enemies[idx].x, enemies[idx].y, enemies[idx].type, 20);
                enemies.erase(enemies.begin() + idx);
            }
        }
    }

    if (!shieldItem.active) {
        shieldSpawnTimer -= dt;
        if (shieldSpawnTimer <= 0.0f) {
            bool validShield = false;
            float sx = 0.0f, sy = 0.0f;

            for (int attempt = 0; attempt < 30; ++attempt) {
                sx = arena.minX + 15.0f + (rand() % (int)(arena.maxX - arena.minX - 30.0f));
                sy = arena.minY + 15.0f + (rand() % (int)(arena.maxY - arena.minY - 30.0f));

                bool overlap = false;
                for (const auto& pillar : pillars) {
                    if (sx >= pillar.box.minX - 12.0f && sx <= pillar.box.maxX + 12.0f &&
                        sy >= pillar.box.minY - 12.0f && sy <= pillar.box.maxY + 12.0f) {
                        overlap = true; break;
                    }
                }
                if (!overlap) { validShield = true; break; }
            }

            if (validShield) {
                shieldItem.x = sx; shieldItem.y = sy;
                shieldItem.active = true; shieldItem.life = 10.0f; shieldItem.angle = 0.0f;
            }
            shieldSpawnTimer = 25.0f;
        }
    }
    else {
        shieldItem.life -= dt;
        float dx = player.x - shieldItem.x; float dy = player.y - shieldItem.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < magnetRadius && dist > 0.001f) {
            float pullSpeed = 160.0f * (1.0f - (dist / magnetRadius) * 0.4f);
            shieldItem.x += (dx / dist) * pullSpeed * dt;
            shieldItem.y += (dy / dist) * pullSpeed * dt;
        }

        if (shieldItem.life <= 0.0f) shieldItem.active = false;
        else if (CheckPlayerVsCircle(player, shieldItem.x, shieldItem.y, shieldItem.radius)) {
            shieldItem.active = false; hasShield = true; shieldTimer = 15.0f;
            SoundManager::PlayRetroBeep(105, 160); TriggerShake(2.0f, 0.12f);
            SpawnItemPickupParticles(shieldItem.x, shieldItem.y, 2);
        }
    }

    targetLockTimer -= dt;
    if (targetLockTimer <= 0.0f && !enemies.empty()) {
        std::vector<int> candidates;
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (!enemies[i].isTargeting && !enemies[i].isPreparingExplosion) candidates.push_back((int)i);
        }

        if (!candidates.empty()) {
            int idx = candidates[rand() % candidates.size()];
            enemies[idx].isTargeting = true; enemies[idx].targetTimer = 1.2f;
            enemies[idx].vx = 0; enemies[idx].vy = 0;

            float dx = player.x - enemies[idx].x; float dy = player.y - enemies[idx].y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 0) {
                enemies[idx].targetDirX = dx / len; enemies[idx].targetDirY = dy / len;
            }
            SoundManager::PlayRetroBeep(80, 100);
        }
        targetLockTimer = 10.0f;
    }

    for (size_t i = 0; i < enemies.size(); ++i) {
        for (size_t j = i + 1; j < enemies.size(); ++j) {
            float dx = enemies[j].x - enemies[i].x; float dy = enemies[j].y - enemies[i].y;
            float distSq = dx * dx + dy * dy;
            float minDist = enemies[i].radius + enemies[j].radius;

            if (distSq < minDist * minDist && distSq > 0.0001f) {
                float dist = std::sqrt(distSq);
                float overlap = minDist - dist;
                float nx = dx / dist; float ny = dy / dist;

                enemies[i].x -= nx * overlap * 0.5f; enemies[i].y -= ny * overlap * 0.5f;
                enemies[j].x += nx * overlap * 0.5f; enemies[j].y += ny * overlap * 0.5f;

                float kx = enemies[i].vx - enemies[j].vx; float ky = enemies[i].vy - enemies[j].vy;
                float p = nx * kx + ny * ky;
                if (p > 0) {
                    float speedI = std::sqrt(enemies[i].vx * enemies[i].vx + enemies[i].vy * enemies[i].vy);
                    float speedJ = std::sqrt(enemies[j].vx * enemies[j].vx + enemies[j].vy * enemies[j].vy);

                    enemies[i].vx -= p * nx; enemies[i].vy -= p * ny;
                    enemies[j].vx += p * nx; enemies[j].vy += p * ny;

                    float newSpeedI = std::sqrt(enemies[i].vx * enemies[i].vx + enemies[i].vy * enemies[i].vy);
                    if (newSpeedI > 0.001f && speedI > 0.001f) {
                        enemies[i].vx = (enemies[i].vx / newSpeedI) * speedI;
                        enemies[i].vy = (enemies[i].vy / newSpeedI) * speedI;
                    }
                    float newSpeedJ = std::sqrt(enemies[j].vx * enemies[j].vx + enemies[j].vy * enemies[j].vy);
                    if (newSpeedJ > 0.001f && speedJ > 0.001f) {
                        enemies[j].vx = (enemies[j].vx / newSpeedJ) * speedJ;
                        enemies[j].vy = (enemies[j].vy / newSpeedJ) * speedJ;
                    }
                }
            }
        }
    }

    for (auto& enemy : enemies) {
        if (enemy.isTargeting) {
            enemy.targetTimer -= effectiveDt;
            if (enemy.targetTimer <= 0.0f) {
                enemy.isTargeting = false;
                enemy.vx = enemy.targetDirX * 60.0f;
                enemy.vy = enemy.targetDirY * 60.0f;
                SoundManager::PlayRetroBeep(72, 150);
            }
        }
        else if (enemy.type == ENEMY_HEXAGON) {
            enemy.hexPauseTimer -= effectiveDt;
            if (enemy.hexPauseTimer <= 0.0f && !enemy.isPreparingExplosion && !enemy.isTargeting) {
                enemy.isPreparingExplosion = true;
                enemy.explosionTimer = 1.2f;
                enemy.savedVx = enemy.vx; enemy.savedVy = enemy.vy;
                enemy.vx = 0.0f; enemy.vy = 0.0f;
                SoundManager::PlayRetroBeep(90, 100);
            }

            if (enemy.isPreparingExplosion) {
                enemy.explosionTimer -= effectiveDt;
                enemy.angle += 12.0f * effectiveDt;

                if (enemy.explosionTimer <= 0.0f) {
                    float explosionRadius = enemy.radius * 5.0f;
                    if (CheckPlayerVsCircle(player, enemy.x, enemy.y, explosionRadius)) TryKillPlayer();

                    SoundManager::PlayRetroBeep(45, 200); TriggerShake(5.0f, 0.25f);
                    SpawnHexExplosionParticles(enemy.x, enemy.y);

                    float newAngle = (float)(rand() % 360) * 3.14159f / 180.0f;
                    float speed = 40.0f;
                    enemy.vx = cosf(newAngle) * speed; enemy.vy = sinf(newAngle) * speed;
                    enemy.isPreparingExplosion = false;
                    enemy.hexPauseTimer = 3.0f + (float)(rand() % 30) / 10.0f;
                }
            }
            else {
                enemy.angle += 1.5f * effectiveDt;
                enemy.x += enemy.vx * effectiveDt; enemy.y += enemy.vy * effectiveDt;
            }
        }
        else {
            enemy.x += enemy.vx * effectiveDt; enemy.y += enemy.vy * effectiveDt;
        }

        if (enemy.type == ENEMY_TRIANGLE && !enemy.isTargeting) {
            enemy.angle += 1.5f * effectiveDt;
            enemy.shootTimer -= effectiveDt;

            if (enemy.shootTimer <= 0.0f) {
                enemy.shootTimer = 2.5f; SoundManager::PlayRetroBeep(85, 60);

                for (int i = 0; i < 3; ++i) {
                    float vAngle = enemy.angle + (i * 120.0f * 3.14159265f / 180.0f);
                    Bullet b;
                    b.x = enemy.x + cosf(vAngle) * enemy.radius;
                    b.y = enemy.y + sinf(vAngle) * enemy.radius;
                    float bSpeed = 110.0f;
                    b.vx = cosf(vAngle) * bSpeed; b.vy = sinf(vAngle) * bSpeed;
                    b.radius = player.radius * 0.6f; b.life = 1.0f;
                    bullets.push_back(b);
                }
            }
        }

        // 아레나 입장 여부 체크 및 내부 경계 처리
        if (!enemy.hasEntered) {
            if (enemy.x - enemy.radius >= arena.minX && enemy.x + enemy.radius <= arena.maxX &&
                enemy.y - enemy.radius >= arena.minY && enemy.y + enemy.radius <= arena.maxY) {
                enemy.hasEntered = true;
            }
        }
        else {
            if (enemy.x - enemy.radius < arena.minX) { enemy.x = arena.minX + enemy.radius; enemy.vx = std::abs(enemy.vx); }
            if (enemy.x + enemy.radius > arena.maxX) { enemy.x = arena.maxX - enemy.radius; enemy.vx = -std::abs(enemy.vx); }
            if (enemy.y - enemy.radius < arena.minY) { enemy.y = arena.minY + enemy.radius; enemy.vy = std::abs(enemy.vy); }
            if (enemy.y + enemy.radius > arena.maxY) { enemy.y = arena.maxY - enemy.radius; enemy.vy = -std::abs(enemy.vy); }
        }

        // --- 적이 네모칸(아레나) 내부로 들어온 경우에만 벽(거대 필러, 소형 필러)과 충돌하도록 수정 ---
        if (enemy.hasEntered) {
            for (const auto& gp : giantPillars) {
                if (gp.state == GIANT_SOLID || gp.state == GIANT_EXTENDING) {
                    if (gp.type == PILLAR_STRAIGHT) {
                        float bx = enemy.x - centerX; float by = enemy.y - centerY;
                        float perpDist = bx * gp.normX + by * gp.normY;
                        float paraDist = bx * gp.dirX + by * gp.dirY;
                        float maxL = (gp.state == GIANT_SOLID) ? 250.0f : gp.currentLength;

                        if (std::abs(perpDist) < (enemy.radius + gp.thickness / 2.0f) &&
                            paraDist >= -250.0f && paraDist <= maxL) {
                            float sign = (perpDist >= 0) ? 1.0f : -1.0f;
                            float overlap = (enemy.radius + gp.thickness / 2.0f) - std::abs(perpDist);
                            enemy.x += gp.normX * sign * overlap;
                            enemy.y += gp.normY * sign * overlap;

                            float dot = enemy.vx * gp.normX + enemy.vy * gp.normY;
                            if ((sign > 0 && dot < 0) || (sign < 0 && dot > 0)) {
                                enemy.vx -= 2.0f * dot * gp.normX;
                                enemy.vy -= 2.0f * dot * gp.normY;
                            }
                        }
                    }
                    else if (gp.type == PILLAR_CURVED) {
                        int numSegs = 20;
                        float maxT = (gp.state == GIANT_SOLID) ? 1.0f : (1.0f - gp.timer / 0.35f);
                        maxT = (std::max)(0.05f, (std::min)(1.0f, maxT));

                        float minDist = 99999.0f;
                        float bestProjX = 0.0f, bestProjY = 0.0f;

                        for (int i = 0; i < numSegs; ++i) {
                            float tA = ((float)i / numSegs) * maxT;
                            float tB = ((float)(i + 1) / numSegs) * maxT;
                            float ax, ay, bx, by, projX, projY;
                            GetBezierPoint(tA, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, ax, ay);
                            GetBezierPoint(tB, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, bx, by);

                            float dist = DistToSegment(enemy.x, enemy.y, ax, ay, bx, by, projX, projY);
                            if (dist < minDist) { minDist = dist; bestProjX = projX; bestProjY = projY; }
                        }

                        if (minDist < (enemy.radius + gp.thickness / 2.0f)) {
                            float diffX = enemy.x - bestProjX; float diffY = enemy.y - bestProjY;
                            float dLen = std::sqrt(diffX * diffX + diffY * diffY);
                            if (dLen > 0.001f) {
                                float nx = diffX / dLen; float ny = diffY / dLen;
                                float overlap = (enemy.radius + gp.thickness / 2.0f) - dLen;
                                enemy.x += nx * overlap; enemy.y += ny * overlap;

                                float dot = enemy.vx * nx + enemy.vy * ny;
                                if (dot < 0.0f) {
                                    enemy.vx -= 2.0f * dot * nx; enemy.vy -= 2.0f * dot * ny;
                                }
                            }
                        }
                    }
                    else if (gp.type == PILLAR_SHAPE) {
                        float scaleFactor = (gp.state == GIANT_SOLID) ? 1.0f : (1.0f - gp.timer / 0.35f);
                        scaleFactor = (std::max)(0.05f, (std::min)(1.0f, scaleFactor));

                        int numVerts = (int)gp.shapeVertices.size();
                        float minDist = 99999.0f;
                        float bestProjX = 0.0f, bestProjY = 0.0f;

                        for (int i = 0; i < numVerts; ++i) {
                            float ax = gp.shapeCenterX + (gp.shapeVertices[i].first - gp.shapeCenterX) * scaleFactor;
                            float ay = gp.shapeCenterY + (gp.shapeVertices[i].second - gp.shapeCenterY) * scaleFactor;
                            float bx = gp.shapeCenterX + (gp.shapeVertices[(i + 1) % numVerts].first - gp.shapeCenterX) * scaleFactor;
                            float by = gp.shapeCenterY + (gp.shapeVertices[(i + 1) % numVerts].second - gp.shapeCenterY) * scaleFactor;

                            float projX, projY;
                            float dist = DistToSegment(enemy.x, enemy.y, ax, ay, bx, by, projX, projY);
                            if (dist < minDist) { minDist = dist; bestProjX = projX; bestProjY = projY; }
                        }

                        if (minDist < (enemy.radius + gp.thickness / 2.0f)) {
                            float diffX = enemy.x - bestProjX; float diffY = enemy.y - bestProjY;
                            float dLen = std::sqrt(diffX * diffX + diffY * diffY);
                            if (dLen > 0.001f) {
                                float nx = diffX / dLen; float ny = diffY / dLen;
                                float overlap = (enemy.radius + gp.thickness / 2.0f) - dLen;
                                enemy.x += nx * overlap; enemy.y += ny * overlap;

                                float dot = enemy.vx * nx + enemy.vy * ny;
                                if (dot < 0.0f) {
                                    enemy.vx -= 2.0f * dot * nx; enemy.vy -= 2.0f * dot * ny;
                                }
                            }
                        }
                    }
                }
            }

            for (auto& pillar : pillars) {
                if (pillar.isActive) {
                    if (HandleEnemyVsAABB(enemy, pillar.box)) {
                        pillar.hp--; SoundManager::PlayRetroBeep(60, 50);
                    }
                }
            }
        }

        if (CheckPlayerVsCircle(player, enemy.x, enemy.y, enemy.radius)) TryKillPlayer();
    }

    for (auto& bullet : bullets) {
        bullet.x += bullet.vx * effectiveDt; bullet.y += bullet.vy * effectiveDt;
        bullet.life -= dt;

        if (bullet.x - bullet.radius < arena.minX || bullet.x + bullet.radius > arena.maxX ||
            bullet.y - bullet.radius < arena.minY || bullet.y + bullet.radius > arena.maxY) {
            bullet.life = 0.0f; SpawnBulletHitParticles(bullet.x, bullet.y); continue;
        }

        for (const auto& pillar : pillars) {
            if (pillar.isActive) {
                float closestX = (std::max)(pillar.box.minX, (std::min)(bullet.x, pillar.box.maxX));
                float closestY = (std::max)(pillar.box.minY, (std::min)(bullet.y, pillar.box.maxY));
                float diffX = bullet.x - closestX; float diffY = bullet.y - closestY;
                if ((diffX * diffX + diffY * diffY) < (bullet.radius * bullet.radius)) {
                    bullet.life = 0.0f; SpawnBulletHitParticles(bullet.x, bullet.y); break;
                }
            }
        }
        if (bullet.life <= 0.0f) continue;

        for (const auto& gp : giantPillars) {
            if (gp.state == GIANT_SOLID || gp.state == GIANT_EXTENDING) {
                if (gp.type == PILLAR_STRAIGHT) {
                    float bx = bullet.x - centerX; float by = bullet.y - centerY;
                    float perpDist = bx * gp.normX + by * gp.normY;
                    float paraDist = bx * gp.dirX + by * gp.dirY;
                    float maxL = (gp.state == GIANT_SOLID) ? 250.0f : gp.currentLength;

                    if (std::abs(perpDist) < (bullet.radius + gp.thickness / 2.0f) &&
                        paraDist >= -250.0f && paraDist <= maxL) {
                        bullet.life = 0.0f; SpawnBulletHitParticles(bullet.x, bullet.y); break;
                    }
                }
                else if (gp.type == PILLAR_CURVED) {
                    int numSegs = 20;
                    float maxT = (gp.state == GIANT_SOLID) ? 1.0f : (1.0f - gp.timer / 0.35f);
                    maxT = (std::max)(0.05f, (std::min)(1.0f, maxT));

                    for (int i = 0; i < numSegs; ++i) {
                        float tA = ((float)i / numSegs) * maxT;
                        float tB = ((float)(i + 1) / numSegs) * maxT;
                        float ax, ay, bx, by, projX, projY;
                        GetBezierPoint(tA, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, ax, ay);
                        GetBezierPoint(tB, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, bx, by);

                        float dist = DistToSegment(bullet.x, bullet.y, ax, ay, bx, by, projX, projY);
                        if (dist < (bullet.radius + gp.thickness / 2.0f)) {
                            bullet.life = 0.0f; SpawnBulletHitParticles(bullet.x, bullet.y); break;
                        }
                    }
                }
                else if (gp.type == PILLAR_SHAPE) {
                    float scaleFactor = (gp.state == GIANT_SOLID) ? 1.0f : (1.0f - gp.timer / 0.35f);
                    scaleFactor = (std::max)(0.05f, (std::min)(1.0f, scaleFactor));

                    int numVerts = (int)gp.shapeVertices.size();
                    for (int i = 0; i < numVerts; ++i) {
                        float ax = gp.shapeCenterX + (gp.shapeVertices[i].first - gp.shapeCenterX) * scaleFactor;
                        float ay = gp.shapeCenterY + (gp.shapeVertices[i].second - gp.shapeCenterY) * scaleFactor;
                        float bx = gp.shapeCenterX + (gp.shapeVertices[(i + 1) % numVerts].first - gp.shapeCenterX) * scaleFactor;
                        float by = gp.shapeCenterY + (gp.shapeVertices[(i + 1) % numVerts].second - gp.shapeCenterY) * scaleFactor;

                        float projX, projY;
                        float dist = DistToSegment(bullet.x, bullet.y, ax, ay, bx, by, projX, projY);
                        if (dist < (bullet.radius + gp.thickness / 2.0f)) {
                            bullet.life = 0.0f; SpawnBulletHitParticles(bullet.x, bullet.y); break;
                        }
                    }
                }
            }
        }
        if (bullet.life <= 0.0f) continue;

        if (CheckPlayerVsCircle(player, bullet.x, bullet.y, bullet.radius)) TryKillPlayer();
    }

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
        [](const Bullet& b) { return b.life <= 0.0f; }), bullets.end());

    pillars.erase(std::remove_if(pillars.begin(), pillars.end(),
        [](const SmallPillar& p) { return p.isActive && p.hp <= 0; }), pillars.end());

    ballSpawnTimer -= dt;
    if (ballSpawnTimer <= 0.0f) {
        int r = rand() % 3;
        EnemyType newType = (r == 0) ? ENEMY_CIRCLE : ((r == 1) ? ENEMY_TRIANGLE : ENEMY_HEXAGON);
        enemies.push_back(SpawnEnemyOutsideArena(newType));
        ballSpawnTimer = 12.0f;
    }
}

void PlayScene::Draw() {
    HDC hdc = g_RenderDC;
    float CX = (arena.minX + arena.maxX) / 2.0f;
    float CY = (arena.minY + arena.maxY) / 2.0f;

    static const int PIXEL_CIRCLE[8][8] = {
        {0, 1, 1, 1, 1, 1, 1, 0},{1, 1, 2, 2, 2, 2, 1, 1},{1, 2, 2, 2, 2, 2, 2, 1},{1, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 2, 1},{1, 2, 2, 2, 2, 2, 2, 1},{1, 1, 2, 2, 2, 2, 1, 1},{0, 1, 1, 1, 1, 1, 1, 0}
    };
    static const int PIXEL_TRIANGLE[7][7] = {
        {0, 0, 0, 1, 0, 0, 0},{0, 0, 1, 2, 1, 0, 0},{0, 0, 1, 2, 1, 0, 0},{0, 1, 2, 2, 2, 1, 0},
        {0, 1, 2, 2, 2, 1, 0},{1, 2, 2, 2, 2, 2, 1},{1, 1, 1, 1, 1, 1, 1}
    };
    static const int PIXEL_HEXAGON[8][8] = {
        {0, 0, 1, 1, 1, 1, 0, 0},{0, 1, 2, 2, 2, 2, 1, 0},{1, 2, 2, 2, 2, 2, 2, 1},{1, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 2, 1},{1, 2, 2, 2, 2, 2, 2, 1},{0, 1, 2, 2, 2, 2, 1, 0},{0, 0, 1, 1, 1, 1, 0, 0}
    };
    static const int PIXEL_STAR[8][8] = {
        {0, 0, 0, 1, 1, 0, 0, 0},{0, 0, 0, 1, 1, 0, 0, 0},{1, 1, 1, 1, 1, 1, 1, 1},{0, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 1, 1, 1, 1, 0, 0},{0, 1, 1, 0, 0, 1, 1, 0},{1, 1, 0, 0, 0, 0, 1, 1},{1, 0, 0, 0, 0, 0, 0, 1}
    };
    static const int PIXEL_SWORD[8][8] = {
        {0, 0, 0, 0, 0, 1, 1, 0},{0, 0, 0, 0, 1, 2, 1, 0},{0, 0, 0, 1, 2, 1, 0, 0},{0, 0, 1, 2, 1, 0, 0, 0},
        {0, 1, 1, 1, 0, 0, 0, 0},{1, 1, 1, 0, 0, 0, 0, 0},{0, 1, 0, 0, 0, 0, 0, 0},{1, 0, 0, 0, 0, 0, 0, 0}
    };
    static const int PIXEL_SHIELD[7][7] = {
        {1, 1, 1, 1, 1, 1, 1},{1, 2, 2, 1, 2, 2, 1},{1, 1, 1, 1, 1, 1, 1},{0, 1, 2, 1, 2, 1, 0},
        {0, 0, 1, 1, 1, 0, 0},{0, 0, 1, 1, 1, 0, 0},{0, 0, 0, 1, 0, 0, 0}
    };

    float disappearProgress = 0.0f;
    if (state == PLAY_STATE_DYING) {
        disappearProgress = (std::min)(1.0f, deathTimer / DEATH_DISAPPEAR_TIME);
    }
    else if (state == PLAY_STATE_GAMEOVER || state == PLAY_STATE_INPUT_INITIAL) {
        disappearProgress = 1.0f;
    }
    float worldScale = 1.0f - disappearProgress;

    auto ScaleX = [&](float x) -> float { return CX + (x - CX) * worldScale; };
    auto ScaleY = [&](float y) -> float { return CY + (y - CY) * worldScale; };

    if (worldScale > 0.001f) {
        DrawPixelRectBorder(hdc, ScaleX(arena.minX), ScaleY(arena.minY), ScaleX(arena.maxX), ScaleY(arena.maxY), 2.0f * worldScale, RGB(20, 20, 30));

        for (const auto& gp : giantPillars) {
            if (gp.state == GIANT_WARNING) {
                HPEN redDotPen = CreatePen(PS_DOT, 1, RGB(220, 30, 30));
                HPEN oldPen = (HPEN)SelectObject(hdc, redDotPen);

                if (gp.type == PILLAR_STRAIGHT) {
                    float halfT = gp.thickness / 2.0f;
                    MoveToEx(hdc, (int)ScaleX(CX - gp.dirX * 250.0f + gp.normX * halfT), (int)ScaleY(CY - gp.dirY * 250.0f + gp.normY * halfT), NULL);
                    LineTo(hdc, (int)ScaleX(CX + gp.dirX * 250.0f + gp.normX * halfT), (int)ScaleY(CY + gp.dirY * 250.0f + gp.normY * halfT));

                    MoveToEx(hdc, (int)ScaleX(CX - gp.dirX * 250.0f - gp.normX * halfT), (int)ScaleY(CY - gp.dirY * 250.0f - gp.normY * halfT), NULL);
                    LineTo(hdc, (int)ScaleX(CX + gp.dirX * 250.0f - gp.normX * halfT), (int)ScaleY(CY + gp.dirY * 250.0f - gp.normY * halfT));
                }
                else if (gp.type == PILLAR_CURVED) {
                    int numSegs = 20;
                    float halfT = gp.thickness / 2.0f;

                    for (int side = -1; side <= 1; side += 2) {
                        for (int i = 0; i < numSegs; ++i) {
                            float tA = (float)i / numSegs; float tB = (float)(i + 1) / numSegs;
                            float ax, ay, bx, by;
                            GetBezierPoint(tA, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, ax, ay);
                            GetBezierPoint(tB, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, bx, by);

                            float dx = bx - ax, dy = by - ay;
                            float len = std::sqrt(dx * dx + dy * dy);
                            if (len < 0.001f) continue;

                            float nx = -dy / len, ny = dx / len;
                            MoveToEx(hdc, (int)ScaleX(ax + nx * halfT * side), (int)ScaleY(ay + ny * halfT * side), NULL);
                            LineTo(hdc, (int)ScaleX(bx + nx * halfT * side), (int)ScaleY(by + ny * halfT * side));
                        }
                    }
                }
                else if (gp.type == PILLAR_SHAPE) {
                    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    int numVerts = (int)gp.shapeVertices.size();
                    std::vector<POINT> pts;
                    for (int i = 0; i < numVerts; ++i) {
                        pts.push_back({ (long)ScaleX(gp.shapeVertices[i].first), (long)ScaleY(gp.shapeVertices[i].second) });
                    }
                    Polygon(hdc, pts.data(), (int)pts.size());
                    SelectObject(hdc, oldBrush);
                }
                SelectObject(hdc, oldPen);
                DeleteObject(redDotPen);
            }
            else if (gp.state == GIANT_EXTENDING || gp.state == GIANT_SOLID) {
                COLORREF pColor = (gp.state == GIANT_EXTENDING) ? RGB(230, 90, 40) : RGB(80, 80, 95);
                HBRUSH pBrush = CreateSolidBrush(pColor);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, pBrush);
                HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));

                if (gp.type == PILLAR_STRAIGHT) {
                    float halfT = gp.thickness / 2.0f;
                    float endL = (gp.state == GIANT_SOLID) ? 250.0f : gp.currentLength;

                    POINT pts[4];
                    pts[0] = { (long)ScaleX(CX - gp.dirX * 250.0f + gp.normX * halfT), (long)ScaleY(CY - gp.dirY * 250.0f + gp.normY * halfT) };
                    pts[1] = { (long)ScaleX(CX - gp.dirX * 250.0f - gp.normX * halfT), (long)ScaleY(CY - gp.dirY * 250.0f - gp.normY * halfT) };
                    pts[2] = { (long)ScaleX(CX + gp.dirX * endL - gp.normX * halfT),    (long)ScaleY(CY + gp.dirY * endL - gp.normY * halfT) };
                    pts[3] = { (long)ScaleX(CX + gp.dirX * endL + gp.normX * halfT),    (long)ScaleY(CY + gp.dirY * endL + gp.normY * halfT) };

                    Polygon(hdc, pts, 4);
                }
                else if (gp.type == PILLAR_CURVED) {
                    int numSegs = 20;
                    float maxT = (gp.state == GIANT_SOLID) ? 1.0f : (1.0f - gp.timer / 0.35f);
                    maxT = (std::max)(0.05f, (std::min)(1.0f, maxT));
                    float halfT = gp.thickness / 2.0f;

                    std::vector<POINT> leftPts, rightPts;
                    for (int i = 0; i <= numSegs; ++i) {
                        float t = ((float)i / numSegs) * maxT;
                        float px, py;
                        GetBezierPoint(t, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, px, py);

                        float tNext = (std::min)(1.0f, t + 0.01f);
                        float npx, npy;
                        GetBezierPoint(tNext, gp.p0x, gp.p0y, gp.p1x, gp.p1y, gp.p2x, gp.p2y, npx, npy);

                        float dx = npx - px, dy = npy - py;
                        float len = std::sqrt(dx * dx + dy * dy);
                        if (len < 0.0001f) { dx = 1.0f; dy = 0.0f; len = 1.0f; }

                        float nx = -dy / len, ny = dx / len;
                        leftPts.push_back({ (long)ScaleX(px + nx * halfT), (long)ScaleY(py + ny * halfT) });
                        rightPts.push_back({ (long)ScaleX(px - nx * halfT), (long)ScaleY(py - ny * halfT) });
                    }

                    for (size_t i = 0; i < leftPts.size() - 1; ++i) {
                        POINT pts[4] = { leftPts[i], rightPts[i], rightPts[i + 1], leftPts[i + 1] };
                        Polygon(hdc, pts, 4);
                    }
                }
                else if (gp.type == PILLAR_SHAPE) {
                    float scaleFactor = (gp.state == GIANT_SOLID) ? 1.0f : (1.0f - gp.timer / 0.35f);
                    scaleFactor = (std::max)(0.05f, (std::min)(1.0f, scaleFactor));

                    HPEN shapePen = CreatePen(PS_SOLID, (int)gp.thickness, pColor);
                    SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    SelectObject(hdc, shapePen);

                    int numVerts = (int)gp.shapeVertices.size();
                    std::vector<POINT> pts;
                    for (int i = 0; i < numVerts; ++i) {
                        float vx = gp.shapeCenterX + (gp.shapeVertices[i].first - gp.shapeCenterX) * scaleFactor;
                        float vy = gp.shapeCenterY + (gp.shapeVertices[i].second - gp.shapeCenterY) * scaleFactor;
                        pts.push_back({ (long)ScaleX(vx), (long)ScaleY(vy) });
                    }
                    Polygon(hdc, pts.data(), (int)pts.size());
                    DeleteObject(shapePen);
                }

                SelectObject(hdc, oldPen);
                SelectObject(hdc, oldBrush);
                DeleteObject(pBrush);
            }
        }

        for (const auto& pillar : pillars) {
            float pcx = (pillar.box.minX + pillar.box.maxX) / 2.0f;
            float pcy = (pillar.box.minY + pillar.box.maxY) / 2.0f;
            float halfW = ((pillar.box.maxX - pillar.box.minX) / 2.0f) * worldScale;
            float halfH = ((pillar.box.maxY - pillar.box.minY) / 2.0f) * worldScale;

            float scx = ScaleX(pcx); float scy = ScaleY(pcy);

            if (pillar.isWarning) {
                HPEN dotPen = CreatePen(PS_DOT, 1, RGB(220, 30, 30));
                HPEN oldPen = (HPEN)SelectObject(hdc, dotPen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

                RECT r = { (int)(scx - halfW), (int)(scy - halfH), (int)(scx + halfW), (int)(scy + halfH) };
                Rectangle(hdc, r.left, r.top, r.right, r.bottom);

                SelectObject(hdc, oldPen);
                SelectObject(hdc, oldBrush);
                DeleteObject(dotPen);
            }
            else {
                RECT r = { (int)(scx - halfW), (int)(scy - halfH), (int)(scx + halfW), (int)(scy + halfH) };
                HBRUSH pillarBrush = CreateSolidBrush(RGB(40, 120, 180));
                FillRect(hdc, &r, pillarBrush); DeleteObject(pillarBrush);

                RECT innerR = { (int)(scx - halfW * 0.6f), (int)(scy - halfH * 0.6f), (int)(scx + halfW * 0.6f), (int)(scy + halfH * 0.6f) };
                HBRUSH innerBrush = CreateSolidBrush(RGB(80, 170, 230));
                FillRect(hdc, &innerR, innerBrush); DeleteObject(innerBrush);
            }
        }

        if (starItem.active && ((starItem.life > 2.0f) || ((int)(starItem.life * 10.0f) % 2 == 0))) {
            DrawPixelGrid(hdc, ScaleX(starItem.x), ScaleY(starItem.y), (const int*)PIXEL_STAR, 8, 8, 1.0f * worldScale, RGB(230, 180, 0), starItem.angle);
        }

        if (swordItem.active && ((swordItem.life > 2.0f) || ((int)(swordItem.life * 10.0f) % 2 == 0))) {
            DrawPixelGrid(hdc, ScaleX(swordItem.x), ScaleY(swordItem.y), (const int*)PIXEL_SWORD, 8, 8, 1.0f * worldScale, RGB(0, 140, 220), swordItem.angle, RGB(220, 160, 0), true);
        }

        if (shieldItem.active && ((shieldItem.life > 2.0f) || ((int)(shieldItem.life * 10.0f) % 2 == 0))) {
            DrawPixelGrid(hdc, ScaleX(shieldItem.x), ScaleY(shieldItem.y), (const int*)PIXEL_SHIELD, 7, 7, 1.1f * worldScale, RGB(20, 110, 220), 0.0f, RGB(255, 255, 255), true);
        }

        for (const auto& p : particles) {
            if (p.isHeart) {
                DrawPixelHeart(hdc, ScaleX(p.x), ScaleY(p.y), p.color, 0.0f, 0.0f, 0.0f, 0);
            }
            else {
                float sz = (0.6f + 0.6f * (1.0f - (p.life / p.maxLife))) * worldScale;
                RECT pr = { (long)(ScaleX(p.x) - sz), (long)(ScaleY(p.y) - sz), (long)(ScaleX(p.x) + sz + 1), (long)(ScaleY(p.y) + sz + 1) };
                HBRUSH pBrush = CreateSolidBrush(p.color);
                FillRect(hdc, &pr, pBrush); DeleteObject(pBrush);
            }
        }
    }

    float currentRadius = 250.0f;
    HRGN fullDarkRgn = NULL;

    if (survivalTime > 10.0f && (state == PLAY_STATE_RUNNING || state == PLAY_STATE_PAUSED)) {
        float progress = (std::min)(1.0f, (survivalTime - 10.0f) / 10.0f);
        currentRadius = 250.0f - (170.0f * progress) + sightBonus;
        if (currentRadius > 250.0f) currentRadius = 250.0f;

        fullDarkRgn = CreateRectRgn(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        HRGN sightRgn = CreateEllipticRgn((int)(player.x - currentRadius), (int)(player.y - currentRadius),
            (int)(player.x + currentRadius), (int)(player.y + currentRadius));
        CombineRgn(fullDarkRgn, fullDarkRgn, sightRgn, RGN_DIFF);

        HBRUSH darkBrush = CreateSolidBrush(RGB(10, 10, 15));
        FillRgn(hdc, fullDarkRgn, darkBrush);
        DeleteObject(darkBrush); DeleteObject(sightRgn);
    }

    if (worldScale > 0.001f) {
        for (const auto& enemy : enemies) {
            float ex = ScaleX(enemy.x); float ey = ScaleY(enemy.y);

            if (enemy.type == ENEMY_HEXAGON && enemy.isPreparingExplosion) {
                float maxExpRad = enemy.radius * 5.0f * worldScale;
                float currentExpRad = maxExpRad * (1.0f - (enemy.explosionTimer / 1.2f));

                HPEN redDotPen = CreatePen(PS_DOT, 1, RGB(220, 30, 30));
                HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
                HPEN oldPen = (HPEN)SelectObject(hdc, redDotPen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, nullBrush);

                Ellipse(hdc, (int)(ex - maxExpRad), (int)(ey - maxExpRad), (int)(ex + maxExpRad), (int)(ey + maxExpRad));

                HBRUSH warnCircleBrush = CreateSolidBrush(RGB(220, 80, 40));
                HPEN noPen = (HPEN)GetStockObject(NULL_PEN);
                SelectObject(hdc, warnCircleBrush); SelectObject(hdc, noPen);

                Ellipse(hdc, (int)(ex - currentExpRad), (int)(ey - currentExpRad), (int)(ex + currentExpRad), (int)(ey + currentExpRad));

                SelectObject(hdc, oldPen); SelectObject(hdc, oldBrush);
                DeleteObject(redDotPen); DeleteObject(warnCircleBrush);
            }

            if (enemy.type == ENEMY_CIRCLE) {
                DrawPixelGrid(hdc, ex, ey, (const int*)PIXEL_CIRCLE, 8, 8, 1.0f * worldScale, RGB(220, 30, 30), 0.0f, RGB(255, 120, 0), true);
            }
            else if (enemy.type == ENEMY_TRIANGLE) {
                DrawPixelGrid(hdc, ex, ey, (const int*)PIXEL_TRIANGLE, 7, 7, 1.1f * worldScale, RGB(180, 0, 150), enemy.angle, RGB(240, 60, 200), true);
            }
            else if (enemy.type == ENEMY_HEXAGON) {
                COLORREF glowColor = enemy.isPreparingExplosion ? RGB(220, 30, 30) : RGB(0, 140, 180);
                COLORREF coreColor = enemy.isPreparingExplosion ? RGB(255, 160, 0) : RGB(40, 200, 220);
                DrawPixelGrid(hdc, ex, ey, (const int*)PIXEL_HEXAGON, 8, 8, 1.0f * worldScale, glowColor, enemy.angle, coreColor, true);
            }

            if (enemy.isTargeting && (state == PLAY_STATE_RUNNING || state == PLAY_STATE_PAUSED)) {
                HPEN redPen = CreatePen(PS_DOT, 1, RGB(220, 20, 20));
                SelectObject(hdc, redPen);
                MoveToEx(hdc, (int)ex, (int)ey, NULL);
                LineTo(hdc, (int)ScaleX(enemy.x + enemy.targetDirX * 300), (int)ScaleY(enemy.y + enemy.targetDirY * 300));
                DeleteObject(redPen);
            }
        }

        HBRUSH bulletBrush = CreateSolidBrush(RGB(255, 180, 0));
        for (const auto& bullet : bullets) {
            float bx = ScaleX(bullet.x); float by = ScaleY(bullet.y);
            float bRad = bullet.radius * 0.8f * worldScale;
            RECT br = { (long)(bx - bRad), (long)(by - bRad), (long)(bx + bRad + 1), (long)(by + bRad + 1) };
            FillRect(hdc, &br, bulletBrush);
        }
        DeleteObject(bulletBrush);
    }

    if (state == PLAY_STATE_DYING) {
        if (deathTimer > DEATH_DISAPPEAR_TIME && deathTimer <= (DEATH_DISAPPEAR_TIME + DEATH_MOVE_TIME)) {
            float t = (deathTimer - DEATH_DISAPPEAR_TIME) / DEATH_MOVE_TIME;
            float easeT = sinf(t * 3.14159265f * 0.5f);
            float deathMaskRadius = 300.0f * (1.0f - easeT);

            if (deathMaskRadius > 1.0f) {
                HRGN fullRgn = CreateRectRgn(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
                HRGN holeRgn = CreateEllipticRgn(
                    (int)(player.x - deathMaskRadius), (int)(player.y - deathMaskRadius),
                    (int)(player.x + deathMaskRadius), (int)(player.y + deathMaskRadius));
                CombineRgn(fullRgn, fullRgn, holeRgn, RGN_DIFF);

                HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
                FillRgn(hdc, fullRgn, blackBrush);

                DeleteObject(blackBrush); DeleteObject(holeRgn); DeleteObject(fullRgn);
            }
        }
        else if (deathTimer > (DEATH_DISAPPEAR_TIME + DEATH_MOVE_TIME)) {
            RECT vRect = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
            HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &vRect, blackBrush); DeleteObject(blackBrush);
        }
    }
    else if (state == PLAY_STATE_GAMEOVER || state == PLAY_STATE_INPUT_INITIAL) {
        RECT vRect = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &vRect, blackBrush); DeleteObject(blackBrush);
    }

    if (state == PLAY_STATE_RUNNING || state == PLAY_STATE_PAUSED) {
        bool showPlayer = (invincibleTimer <= 0.0f) || ((int)(invincibleTimer * 20.0f) % 2 == 0);

        if (showPlayer) {
            if (hasShield && ((shieldTimer > 2.0f) || ((int)(shieldTimer * 10.0f) % 2 == 0))) {
                DrawPixelRectBorder(hdc, ScaleX(player.x - 6), ScaleY(player.y - 6), ScaleX(player.x + 6), ScaleY(player.y + 6), 1.0f, RGB(0, 140, 240));
            }

            COLORREF heartColor = (invincibleTimer > 0.0f) ? RGB(230, 180, 0) : (player.isDashing ? RGB(0, 180, 240) : RGB(0, 180, 80));
            DrawPixelHeart(hdc, player.x, player.y, heartColor);
        }
    }
    else if (state == PLAY_STATE_DYING) {
        if (deathTimer <= DEATH_DISAPPEAR_TIME) {
            if ((int)(deathTimer * 20.0f) % 2 == 0) {
                DrawPixelHeart(hdc, player.x, player.y, RGB(220, 30, 30));
            }
        }
        else if (deathTimer <= DEATH_DISAPPEAR_TIME + DEATH_MOVE_TIME) {
            DrawPixelHeart(hdc, player.x, player.y, RGB(220, 30, 30));
        }
        else if (deathTimer > (DEATH_DISAPPEAR_TIME + DEATH_MOVE_TIME + DEATH_PAUSE_TIME)) {
            float splitProgress = (deathTimer - DEATH_DISAPPEAR_TIME - DEATH_MOVE_TIME - DEATH_PAUSE_TIME) / DEATH_SPLIT_TIME;
            if (splitProgress > 1.0f) splitProgress = 1.0f;

            float sX = (float)(rand() % 3 - 1); float sY = (float)(rand() % 3 - 1);
            float leftX = -splitProgress * 12.0f + sX; float leftY = splitProgress * 10.0f + sY; float leftRot = -splitProgress * 0.45f;
            float rightX = splitProgress * 12.0f + sX; float rightY = splitProgress * 10.0f + sY; float rightRot = splitProgress * 0.45f;

            COLORREF deadColor = RGB(220, 30, 30);
            DrawPixelHeart(hdc, CX, CY, deadColor, leftX, leftY, leftRot, 1);
            DrawPixelHeart(hdc, CX, CY, deadColor, rightX, rightY, rightRot, 2);

            DrawPixelText(hdc, "GAME OVER", CX, 85, 2.5f, RGB(220, 30, 30));
        }
    }
    else if (state == PLAY_STATE_INPUT_INITIAL) {
        COLORREF deadColor = RGB(220, 30, 30);
        DrawPixelHeart(hdc, CX, CY - 35, deadColor, -12.0f, 10.0f, -0.45f, 1);
        DrawPixelHeart(hdc, CX, CY - 35, deadColor, 12.0f, 10.0f, 0.45f, 2);

        DrawPixelText(hdc, "ENTER INITIAL", CX, CY + 10, 1.5f, RGB(220, 30, 30));
        DrawPixelText(hdc, playerInitial, CX, CY + 45, 2.0f, RGB(255, 255, 255));
        DrawPixelText(hdc, "PRESS ENTER TO SAVE", CX, CY + 80, 1.0f, RGB(150, 150, 150));
    }
    else if (state == PLAY_STATE_GAMEOVER) {
        DrawPixelText(hdc, "SCORE SAVED!", CX, CY - 20, 1.8f, RGB(0, 180, 100));
        DrawPixelText(hdc, "PRESS SPACE FOR NEXT GAME", CX, CY + 30, 1.2f, RGB(200, 200, 200));
    }

    if (state == PLAY_STATE_RUNNING || state == PLAY_STATE_PAUSED) {
        std::string dashStatus = (player.dashCooldown <= 0.0f) ? "READY" : std::to_string((int)(player.dashCooldown + 0.9f)) + "S";
        std::string shieldStatus = hasShield ? "SHIELD: " + std::to_string((int)(shieldTimer + 0.9f)) + "S  |  " : "";
        std::string timeStr = "TIME: " + std::to_string((int)survivalTime) + "S  |  " + shieldStatus + "DASH: " + dashStatus;

        DrawPixelText(hdc, timeStr, CX, 15, 1.0f, RGB(20, 20, 30));

        if (fullDarkRgn != NULL) {
            SelectClipRgn(hdc, fullDarkRgn);
            DrawPixelText(hdc, timeStr, CX, 15, 1.0f, RGB(245, 245, 250));
            SelectClipRgn(hdc, NULL);
        }
    }

    if (fullDarkRgn != NULL) DeleteObject(fullDarkRgn);

    if (state == PLAY_STATE_PAUSED) {
        RECT pBox = { 110, 65, 290, 160 };
        HBRUSH pBgBrush = CreateSolidBrush(RGB(245, 245, 250));
        SelectObject(hdc, pBgBrush); SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, pBox.left, pBox.top, pBox.right, pBox.bottom);
        DeleteObject(pBgBrush);

        DrawPixelRectBorder(hdc, 110, 65, 290, 160, 1.5f, RGB(20, 20, 30));
        DrawPixelText(hdc, "PAUSED", CX, 82, 2.0f, RGB(20, 20, 30));

        RECT resumeBtn = { 125, 100, 275, 125 };
        bool hoverResume = (g_MouseX >= resumeBtn.left && g_MouseX <= resumeBtn.right && g_MouseY >= resumeBtn.top && g_MouseY <= resumeBtn.bottom);
        HBRUSH rBrush = CreateSolidBrush(hoverResume ? RGB(0, 180, 90) : RGB(220, 220, 230));
        FillRect(hdc, &resumeBtn, rBrush); DeleteObject(rBrush);
        DrawPixelText(hdc, "RESUME", CX, 112.5f, 1.3f, hoverResume ? RGB(255, 255, 255) : RGB(20, 20, 30));

        RECT quitBtn = { 125, 132, 275, 157 };
        bool hoverQuit = (g_MouseX >= quitBtn.left && g_MouseX <= quitBtn.right && g_MouseY >= quitBtn.top && g_MouseY <= quitBtn.bottom);
        HBRUSH qBrush = CreateSolidBrush(hoverQuit ? RGB(220, 40, 40) : RGB(220, 220, 230));
        FillRect(hdc, &quitBtn, qBrush); DeleteObject(qBrush);
        DrawPixelText(hdc, "END RELAY", CX, 144.5f, 1.3f, hoverQuit ? RGB(255, 255, 255) : RGB(20, 20, 30));
    }
}

void PlayScene::Release() {
    enemies.clear();
    bullets.clear();
    pillars.clear();
    giantPillars.clear();
    particles.clear();
}

void PlayScene::OnKeyDown(WPARAM wParam) {
    if (state == PLAY_STATE_INPUT_INITIAL) {
        if (wParam >= 'A' && wParam <= 'Z') {
            if (playerInitial.length() < 3) {
                playerInitial += (char)wParam;
            }
            else {
                playerInitial[2] = (char)wParam;
            }
            SoundManager::PlayRetroBeep(100, 50);
        }
        else if (wParam == VK_BACK) {
            if (!playerInitial.empty()) {
                playerInitial.pop_back();
                SoundManager::PlayRetroBeep(60, 50);
            }
        }
        else if (wParam == VK_RETURN) {
            if (playerInitial.empty()) playerInitial = "AAA";
            AddRank(playerInitial, (int)survivalTime);
            state = PLAY_STATE_GAMEOVER;
            SoundManager::PlayRetroBeep(90, 80);
        }
        return;
    }

    if (wParam == VK_ESCAPE) {
        if (state == PLAY_STATE_RUNNING) {
            state = PLAY_STATE_PAUSED;
            SoundManager::PlayRetroBeep(75, 80);
        }
        else if (state == PLAY_STATE_PAUSED) {
            state = PLAY_STATE_RUNNING;
            SoundManager::PlayRetroBeep(90, 80);
        }
    }
    else if (wParam == VK_SPACE) {
        if (state == PLAY_STATE_GAMEOVER) {
            SceneManager::GetInstance().ChangeScene((IGameScene*)new TitleScene());
            SoundManager::PlayRetroBeep(70, 80);
        }
        else if (state == PLAY_STATE_RUNNING) {
            if (player.dashCooldown <= 0.0f) {
                player.isDashing = true;
                player.dashDuration = 0.18f;
                player.dashCooldown = 1.2f;
                SoundManager::PlayRetroBeep(90, 80);
                TriggerShake(2.0f, 0.15f);
            }
        }
    }
}

void PlayScene::OnLButtonDown(int mx, int my) {
    if (state == PLAY_STATE_PAUSED) {
        if (mx >= 125 && mx <= 275 && my >= 100 && my <= 125) {
            state = PLAY_STATE_RUNNING;
            SoundManager::PlayRetroBeep(90, 80);
        }
        else if (mx >= 125 && mx <= 275 && my >= 132 && my <= 157) {
           
           
            gameoverchecker = true;                                                    // 👈 이걸로 교체!
            Score = survivalTime;
        }
    }
}



LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    RECT rc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE);
    int windowWidth = rc.right - rc.left;
    int windowHeight = rc.bottom - rc.top;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        100, 100, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

// --- INVEMA.cpp 맨 밑에 붙여넣기 ---
namespace GameA {
    HWND g_hWndA = NULL;

    void Init(HWND hWnd) {
        gameoverchecker = false;
        g_hWndA = hWnd;
      
        SceneManager::GetInstance().ChangeScene(new TitleScene());
    }

    void Release() {
        SceneManager::GetInstance().ChangeScene(nullptr);
    }

    void Update() {
        static LARGE_INTEGER freq, lastTime;
        static bool initTime = false;
        if (!initTime) { QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&lastTime); initTime = true; }
        LARGE_INTEGER currentTime; QueryPerformanceCounter(&currentTime);
        float dt = (float)(currentTime.QuadPart - lastTime.QuadPart) / freq.QuadPart;
        lastTime = currentTime;
        if (dt > 0.1f) dt = 0.1f;
        g_DeltaTime = dt;

        SceneManager::GetInstance().Update();
    }

    void Draw() {
        if (!g_hWndA) return;
        HDC hdc = GetDC(g_hWndA);
        RECT rect; GetClientRect(g_hWndA, &rect);
        int winW = rect.right - rect.left; int winH = rect.bottom - rect.top;

        HDC memDC = CreateCompatibleDC(hdc); HBITMAP memBmp = CreateCompatibleBitmap(hdc, winW, winH);
        HBITMAP oldMemBmp = (HBITMAP)SelectObject(memDC, memBmp);
        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0)); FillRect(memDC, &rect, blackBrush); DeleteObject(blackBrush);

        HDC vDC = CreateCompatibleDC(hdc); HBITMAP vBmp = CreateCompatibleBitmap(hdc, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        HBITMAP oldVBmp = (HBITMAP)SelectObject(vDC, vBmp);
        HBRUSH bgBrush = CreateSolidBrush(RGB(242, 242, 246)); RECT vRect = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        FillRect(vDC, &vRect, bgBrush); DeleteObject(bgBrush);

        g_RenderDC = vDC;
        SceneManager::GetInstance().Draw();

        int drawW = 800; int drawH = 450;
        int offsetX = (winW - drawW) / 2; int offsetY = (winH - drawH) / 2;

        SetStretchBltMode(memDC, COLORONCOLOR);
        StretchBlt(memDC, offsetX, offsetY, drawW, drawH, vDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);
        BitBlt(hdc, 0, 0, winW, winH, memDC, 0, 0, SRCCOPY);

        SelectObject(vDC, oldVBmp); DeleteObject(vBmp); DeleteDC(vDC);
        SelectObject(memDC, oldMemBmp); DeleteObject(memBmp); DeleteDC(memDC); ReleaseDC(g_hWndA, hdc);
    }

    void InputKey(WPARAM wParam) { SceneManager::GetInstance().OnKeyDown(wParam); }
    void InputMouseClick(int mx, int my) { SceneManager::GetInstance().OnLButtonDown(mx / 2, my / 2); }
    void InputMouseMove(int mx, int my) { g_MouseX = mx / 2; g_MouseY = my / 2; }
    bool IsForceEnd() { return isForceEndRelay; } // ⭐ 추가
    bool IsGameOver() { return gameoverchecker; }
    int GetScore() { return (int)Score; }
}
