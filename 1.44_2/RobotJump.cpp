#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <mmsystem.h>
#include <vector>
#include <cmath>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

using namespace DirectX;

template <class T> void SafeRelease(T** ppT) {
    if (*ppT) { (*ppT)->Release(); *ppT = NULL; }
}

struct Vertex {
    XMFLOAT3 pos;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
};

struct ConstantBuffer {
    XMMATRIX WVP;
    XMMATRIX World;
    XMFLOAT4 Color;
};

enum GameState {
    STATE_TITLE,
    STATE_TUNING,
    STATE_GAMEPLAY
};

enum PlatformType {
    PLATFORM_NORMAL,
    PLATFORM_BOOST,
    PLATFORM_RISK,
    PLATFORM_BLACK
};

enum ItemType {
    ITEM_NONE,
    ITEM_HIGH_JUMP,
    ITEM_FLY,
    ITEM_GUN
};

struct Projectile {
    bool active;
    XMFLOAT3 pos;
    XMFLOAT3 dir;
    float speed;
    bool isEnemyBolt;
};

struct SideEnemy {
    bool active;
    XMFLOAT3 pos;
    float timer;
    float shootTimer;
    int hp;
    float hitFlash;
};

struct JumpPlatform {
    XMFLOAT3 pos;
    XMFLOAT3 scale;
    XMFLOAT3 initPos;
    float moveSpeed;
    float moveAmp;
    int moveAxis;
    XMFLOAT4 color;
    PlatformType type;

    bool hasItem;
    ItemType itemType;
    bool itemActive;

    bool isWarning;
    float warningTimer;
    bool isDestroyed;
    bool isMissileStriking;
    float missileAnimTimer;
    XMFLOAT3 missileStrikePos;

    bool isCracked;
    bool isCracking;
    float crackTimer;
};

struct TuningParam {
    const wchar_t* name;
    float* pValue;
    float minVal;
    float maxVal;
    float step;
    const wchar_t* unit;
};
namespace GameC{
    bool gameovercheck;
    bool isForceEndRelay = false;
float g_moveSpeed = 8.0f;
float g_normalJumpPower = 10.0f;
float g_highJumpPower = 17.0f;
float g_flySpeed = 6.5f;
float g_highJumpDuration = 6.0f;
float g_flyDuration = 4.0f;
float g_bulletSpeed = 95.0f;
float g_enemyShootInterval = 3.8f;
float g_stunDuration = 1.0f;

int g_selectedParam = 0;
const int PARAM_COUNT = 9;

XMFLOAT3 g_playerPos = { 0.0f, 2.0f, 0.0f };
XMFLOAT3 g_prevPlayerPos = { 0.0f, 2.0f, 0.0f };
XMFLOAT3 g_playerVel = { 0.0f, 0.0f, 0.0f };
bool g_playerGrounded = false;
float g_timeSinceGrounded = 0.0f;
float g_camYaw = 0.0f;
float g_camPitch = 0.2f;
int g_jumpScore = 0;

bool g_isJumpGameOver = false;
std::vector<JumpPlatform> g_platforms;
float g_lastSpawnZ = 0.0f;

float g_stunTimer = 0.0f;
int g_ammoCount = 0;
bool g_prevShootKey = false;
bool g_isFpsAiming = false;

SideEnemy g_sideEnemy = { false, {0,0,0}, 0.0f, 0.0f, 2, 0.0f };
float g_enemySpawnCooldown = 15.0f;
std::vector<Projectile> g_projectiles;

float g_missileCooldown = 0.0f;
float g_camShakeTimer = 0.0f;
float g_camShakeIntensity = 0.0f;

float g_highJumpTimer = 0.0f;
float g_flyTimer = 0.0f;

GameState g_gameState = STATE_TITLE;
float g_sceneTime = 0.0f;

HMIDIOUT g_hMidi = NULL;
float g_bgmTimer = 0.0f;
int g_bgmStep = 0;

LARGE_INTEGER g_timerFreq;
LARGE_INTEGER g_lastTime;

HWND                    g_hWnd = NULL;
ID3D11Device* g_pDevice = NULL;
ID3D11DeviceContext* g_pContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRTV = NULL;
ID3D11Texture2D* g_pDepthBuffer = NULL;
ID3D11DepthStencilView* g_pDSV = NULL;

ID3D11VertexShader* g_pVS = NULL;
ID3D11PixelShader* g_pPS = NULL;
ID3D11InputLayout* g_pInputLayout = NULL;
ID3D11Buffer* g_pVertexBuffer = NULL;
ID3D11Buffer* g_pIndexBuffer = NULL;
ID3D11Buffer* g_pConstantBuffer = NULL;

ID3D11ShaderResourceView* g_pTextureSRV = NULL;
ID3D11SamplerState* g_pSamplerState = NULL;

ID3D11Texture2D* g_pUITexture = NULL;
ID3D11ShaderResourceView* g_pUISRV = NULL;
ID3D11BlendState* g_pUIBlendState = NULL;
ID3D11Buffer* g_pUIVertexBuffer = NULL;
ID3D11Buffer* g_pUIIndexBuffer = NULL;

HDC                     g_hMemDC = NULL;
HBITMAP                 g_hUIBitmap = NULL;
void* g_pUIBits = nullptr;
HFONT                   g_hFontTitle = NULL;
HFONT                   g_hFontBig = NULL;
HFONT                   g_hFontMed = NULL;
HFONT                   g_hFontSub = NULL;

BYTE g_indexImage16x16[16 * 16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
    0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

const char* g_shaderSource = R"(
cbuffer ConstantBuffer : register(b0) {
    matrix WVP;
    matrix World;
    float4 Color;
};

Texture2D tex16x16 : register(t0);
SamplerState pointSampler : register(s0);

struct VS_INPUT {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(float4(input.pos, 1.0f), WVP);
    output.normal = mul(input.normal, (float3x3)World);
    output.uv = input.uv;
    output.color = Color;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    float3 lightDir = normalize(float3(0.4f, 1.0f, -0.4f));
    float diff = max(dot(normalize(input.normal), lightDir), 0.35f);
    float4 texColor = tex16x16.Sample(pointSampler, input.uv);
    return float4(texColor.rgb * input.color.rgb * diff, texColor.a * input.color.a);
}
)";

void InitAudio() {
    if (midiOutOpen(&g_hMidi, MIDI_MAPPER, 0, 0, 0) == MMSYSERR_NOERROR) {
        midiOutShortMsg(g_hMidi, (10 << 8) | 0xC0);
        midiOutShortMsg(g_hMidi, (8 << 8) | 0xC1);
    }
}

void PlayMidiNote(BYTE channel, BYTE note, BYTE velocity) {
    if (!g_hMidi) return;
    DWORD msg = (velocity << 16) | (note << 8) | (0x90 | (channel & 0x0F));
    midiOutShortMsg(g_hMidi, msg);
}

void UpdateBGM(float dt) {
    g_bgmTimer += dt;
    if (g_bgmTimer >= 0.32f) {
        g_bgmTimer -= 0.32f;
        static const int melody[] = { 60, 64, 67, 72, 71, 67, 64, 62, 59, 62, 67, 71, 69, 67, 64, 60 };
        PlayMidiNote(0, melody[g_bgmStep % 16], 70);
        g_bgmStep++;
    }
}

void InitTimer() {
    QueryPerformanceFrequency(&g_timerFreq);
    QueryPerformanceCounter(&g_lastTime);
}

float GetDeltaTime() {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    float dt = (float)(currentTime.QuadPart - g_lastTime.QuadPart) / (float)g_timerFreq.QuadPart;
    g_lastTime = currentTime;
    return dt;
}

void TriggerCameraShake(float duration, float intensity) {
    g_camShakeTimer = duration;
    g_camShakeIntensity = intensity;
}

void SpawnJumpPlatformCluster(float zPos) {
    int clusterCount = 2 + (rand() % 2);
    float difficulty = min(1.0f, zPos / 200.0f);

    int normalWeight = (int)(80.0f - 50.0f * difficulty);
    int boostWeight = 10;
    int riskWeight = 4;

    for (int i = 0; i < clusterCount; ++i) {
        JumpPlatform p;
        float xOffset = ((float)i - (clusterCount - 1) * 0.5f) * 4.8f + ((rand() % 100) / 100.0f - 0.5f) * 1.2f;
        float yOffset = ((rand() % 100) / 100.0f - 0.5f) * 2.2f;

        p.pos = XMFLOAT3(xOffset, yOffset, zPos + ((rand() % 100) / 100.0f - 0.5f) * 1.0f);
        p.initPos = p.pos;
        p.hasItem = false;
        p.itemType = ITEM_NONE;
        p.itemActive = false;

        p.isWarning = false;
        p.warningTimer = 0.0f;
        p.isDestroyed = false;
        p.isMissileStriking = false;
        p.missileAnimTimer = 0.0f;
        p.missileStrikePos = XMFLOAT3(0, 0, 0);

        p.isCracked = (rand() % 100 < 5);
        p.isCracking = false;
        p.crackTimer = 0.0f;

        int roll = rand() % 100;
        if (roll < normalWeight) {
            p.type = PLATFORM_NORMAL;
            p.scale = XMFLOAT3(2.5f, 0.4f, 2.5f);
            p.color = XMFLOAT4(0.2f, 0.6f, 0.9f, 1.0f);

            if (rand() % 100 < 4) {
                p.hasItem = true;
                p.itemActive = true;
                p.itemType = ITEM_GUN;
            }
        }
        else if (roll < normalWeight + boostWeight) {
            p.type = PLATFORM_BOOST;
            p.scale = XMFLOAT3(2.2f, 0.4f, 2.2f);
            p.color = XMFLOAT4(0.95f, 0.85f, 0.1f, 1.0f);
        }
        else if (roll < normalWeight + boostWeight + riskWeight) {
            p.type = PLATFORM_RISK;
            p.scale = XMFLOAT3(1.5f, 0.4f, 1.5f);
            p.color = XMFLOAT4(0.95f, 0.25f, 0.25f, 1.0f);

            p.hasItem = true;
            p.itemActive = true;
            int itemRoll = rand() % 3;
            p.itemType = (itemRoll == 0) ? ITEM_HIGH_JUMP : ((itemRoll == 1) ? ITEM_FLY : ITEM_GUN);
        }
        else {
            p.type = PLATFORM_BLACK;
            p.scale = XMFLOAT3(1.5f, 0.4f, 1.5f);
            p.color = XMFLOAT4(0.15f, 0.15f, 0.18f, 1.0f);
        }

        p.moveSpeed = 1.0f + (rand() % 100) / 50.0f;
        p.moveAmp = 1.2f + (rand() % 100) / 50.0f;
        p.moveAxis = rand() % 3;
        g_platforms.push_back(p);
    }
}

void InitJumpGame() {
    g_playerPos = XMFLOAT3(0.0f, 2.0f, 0.0f);
    g_prevPlayerPos = g_playerPos;
    g_playerVel = XMFLOAT3(0.0f, 0.0f, 0.0f);
    g_playerGrounded = false;
    g_timeSinceGrounded = 0.0f;
    g_camYaw = 0.0f;
    g_camPitch = 0.2f;
    g_jumpScore = 0;
    g_isJumpGameOver = false;

    g_stunTimer = 0.0f;
    g_ammoCount = 0;
    g_prevShootKey = false;
    g_isFpsAiming = false;

    g_sideEnemy.active = false;
    g_enemySpawnCooldown = 15.0f;
    g_projectiles.clear();

    g_missileCooldown = 2.0f;
    g_camShakeTimer = 0.0f;
    g_camShakeIntensity = 0.0f;
    g_highJumpTimer = 0.0f;
    g_flyTimer = 0.0f;

    g_platforms.clear();

    JumpPlatform startP;
    startP.pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
    startP.scale = XMFLOAT3(4.5f, 0.5f, 4.5f);
    startP.initPos = startP.pos;
    startP.moveSpeed = 0.0f;
    startP.moveAmp = 0.0f;
    startP.moveAxis = 0;
    startP.color = XMFLOAT4(0.2f, 0.8f, 0.3f, 1.0f);
    startP.type = PLATFORM_NORMAL;
    startP.hasItem = false;
    startP.isWarning = false;
    startP.warningTimer = 0.0f;
    startP.isDestroyed = false;
    startP.isMissileStriking = false;
    startP.missileAnimTimer = 0.0f;
    startP.missileStrikePos = XMFLOAT3(0, 0, 0);
    startP.isCracked = false;
    startP.isCracking = false;
    startP.crackTimer = 0.0f;
    g_platforms.push_back(startP);

    g_lastSpawnZ = 0.0f;
    for (int i = 0; i < 12; ++i) {
        g_lastSpawnZ += 5.0f + (rand() % 100) / 50.0f;
        SpawnJumpPlatformCluster(g_lastSpawnZ);
    }
}

void UpdateJumpGame(float dt) {
    if (g_isJumpGameOver) return;

    g_prevPlayerPos = g_playerPos;

    if (g_camShakeTimer > 0.0f) g_camShakeTimer -= dt;
    if (g_highJumpTimer > 0.0f) g_highJumpTimer -= dt;
    if (g_flyTimer > 0.0f) g_flyTimer -= dt;
    if (g_stunTimer > 0.0f) g_stunTimer -= dt;

    g_isFpsAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) && (g_ammoCount > 0);

    if (!g_sideEnemy.active && g_playerPos.z >= 20.0f) {
        g_enemySpawnCooldown -= dt;
        if (g_enemySpawnCooldown <= 0.0f) {
            g_enemySpawnCooldown = 22.0f + (rand() % 10);
            g_sideEnemy.active = true;
            g_sideEnemy.hp = 2;
            g_sideEnemy.timer = 10.0f;
            g_sideEnemy.shootTimer = 1.5f;
            g_sideEnemy.hitFlash = 0.0f;

            float sideX = (rand() % 2 == 0) ? 12.0f : -12.0f;
            g_sideEnemy.pos = XMFLOAT3(sideX, g_playerPos.y + 2.0f, g_playerPos.z + 6.0f);
            PlayMidiNote(9, 82, 120);
        }
    }

    if (g_sideEnemy.active) {
        g_sideEnemy.timer -= dt;
        if (g_sideEnemy.hitFlash > 0.0f) g_sideEnemy.hitFlash -= dt;

        float targetX = (g_sideEnemy.pos.x > 0) ? 11.0f : -11.0f;
        float targetY = g_playerPos.y + 1.5f + sinf(g_sceneTime * 3.0f) * 0.8f;
        float targetZ = g_playerPos.z + 3.0f;

        g_sideEnemy.pos.x += (targetX - g_sideEnemy.pos.x) * 3.0f * dt;
        g_sideEnemy.pos.y += (targetY - g_sideEnemy.pos.y) * 4.0f * dt;
        g_sideEnemy.pos.z += (targetZ - g_sideEnemy.pos.z) * 5.0f * dt;

        g_sideEnemy.shootTimer -= dt;
        if (g_sideEnemy.shootTimer <= 0.0f) {
            g_sideEnemy.shootTimer = g_enemyShootInterval;

            Projectile p;
            p.active = true;
            p.isEnemyBolt = true;
            p.pos = g_sideEnemy.pos;
            p.speed = 15.0f;

            XMVECTOR ePos = XMLoadFloat3(&g_sideEnemy.pos);
            XMVECTOR pTarget = XMLoadFloat3(&g_playerPos);
            XMVECTOR dir = XMVector3Normalize(pTarget - ePos);
            XMStoreFloat3(&p.dir, dir);

            g_projectiles.push_back(p);
            PlayMidiNote(9, 77, 100);
        }

        if (g_sideEnemy.timer <= 0.0f) {
            g_sideEnemy.active = false;
        }
    }

    bool shootPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState('F') & 0x8000);
    if (shootPressed && !g_prevShootKey && g_ammoCount > 0 && g_stunTimer <= 0.0f) {
        g_ammoCount--;

        Projectile bullet;
        bullet.active = true;
        bullet.isEnemyBolt = false;
        bullet.pos = XMFLOAT3(g_playerPos.x, g_playerPos.y + (g_isFpsAiming ? 0.5f : 0.2f), g_playerPos.z);
        bullet.speed = g_bulletSpeed;

        XMVECTOR camDir = XMVectorSet(cosf(g_camPitch) * sinf(g_camYaw), sinf(g_camPitch), cosf(g_camPitch) * cosf(g_camYaw), 0.0f);
        XMStoreFloat3(&bullet.dir, XMVector3Normalize(camDir));

        g_projectiles.push_back(bullet);
        PlayMidiNote(9, 38, 127);
        TriggerCameraShake(0.12f, 0.25f);
    }
    g_prevShootKey = shootPressed;

    for (auto& proj : g_projectiles) {
        if (!proj.active) continue;

        proj.pos.x += proj.dir.x * proj.speed * dt;
        proj.pos.y += proj.dir.y * proj.speed * dt;
        proj.pos.z += proj.dir.z * proj.speed * dt;

        if (proj.isEnemyBolt) {
            float dx = proj.pos.x - g_playerPos.x;
            float dy = proj.pos.y - g_playerPos.y;
            float dz = proj.pos.z - g_playerPos.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < 1.1f * 1.1f) {
                proj.active = false;
                g_stunTimer = g_stunDuration;
                TriggerCameraShake(0.35f, 0.4f);
                PlayMidiNote(9, 45, 127);
            }
        }
        else if (!proj.isEnemyBolt && g_sideEnemy.active) {
            float dx = proj.pos.x - g_sideEnemy.pos.x;
            float dy = proj.pos.y - g_sideEnemy.pos.y;
            float dz = proj.pos.z - g_sideEnemy.pos.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < 2.0f * 2.0f) {
                proj.active = false;
                g_sideEnemy.hp--;
                g_sideEnemy.hitFlash = 0.2f;
                PlayMidiNote(9, 62, 120);

                if (g_sideEnemy.hp <= 0) {
                    g_sideEnemy.active = false;
                    TriggerCameraShake(0.5f, 0.7f);
                    PlayMidiNote(9, 49, 127);
                }
            }
        }

        if (fabsf(proj.pos.z - g_playerPos.z) > 100.0f) proj.active = false;
    }

    if (g_playerPos.z >= 100.0f) {
        g_missileCooldown -= dt;
        if (g_missileCooldown <= 0.0f) {
            g_missileCooldown = 3.0f + (rand() % 20) / 10.0f;

            std::vector<int> targetableIndices;
            for (size_t i = 0; i < g_platforms.size(); ++i) {
                const auto& p = g_platforms[i];
                if (!p.isDestroyed && !p.isWarning && !p.isMissileStriking) {
                    if (p.pos.z > g_playerPos.z + 8.0f && p.pos.z < g_playerPos.z + 32.0f) {
                        targetableIndices.push_back((int)i);
                    }
                }
            }

            if (!targetableIndices.empty()) {
                int chosenIdx = targetableIndices[rand() % targetableIndices.size()];
                float chosenZ = g_platforms[chosenIdx].pos.z;

                bool clusterAlreadyTargeted = false;
                for (const auto& p : g_platforms) {
                    if ((p.isWarning || p.isDestroyed) && fabsf(p.pos.z - chosenZ) < 2.5f) {
                        clusterAlreadyTargeted = true;
                        break;
                    }
                }

                if (!clusterAlreadyTargeted) {
                    g_platforms[chosenIdx].isWarning = true;
                    g_platforms[chosenIdx].warningTimer = 1.8f;
                    PlayMidiNote(9, 80, 110);
                }
            }
        }
    }

    for (auto& p : g_platforms) {
        if (p.isCracking && !p.isDestroyed) {
            p.crackTimer -= dt;
            if (p.crackTimer <= 0.0f) {
                p.isDestroyed = true;
                p.isCracking = false;
                PlayMidiNote(9, 41, 100);
            }
        }

        if (p.isWarning) {
            p.warningTimer -= dt;
            if (p.warningTimer <= 0.0f) {
                p.isWarning = false;
                p.isDestroyed = true;
                p.isMissileStriking = true;
                p.missileAnimTimer = 0.6f;
                p.missileStrikePos = p.pos;

                float dx = g_playerPos.x - p.pos.x;
                float dy = g_playerPos.y - p.pos.y;
                float dz = g_playerPos.z - p.pos.z;
                if (dx * dx + dy * dy + dz * dz < 4.5f * 4.5f) {
                    g_stunTimer = g_stunDuration;
                }

                TriggerCameraShake(0.45f, 0.6f);
                PlayMidiNote(9, 38, 127);
            }
        }

        if (p.isMissileStriking) {
            p.missileAnimTimer -= dt;
            if (p.missileAnimTimer <= 0.0f) {
                p.isMissileStriking = false;
            }
        }

        if (p.isDestroyed) continue;

        if (p.moveAxis == 1) {
            p.pos.x = p.initPos.x + sinf(g_sceneTime * p.moveSpeed) * p.moveAmp;
        }
        else if (p.moveAxis == 2) {
            p.pos.y = p.initPos.y + sinf(g_sceneTime * p.moveSpeed) * p.moveAmp * 0.5f;
        }

        if (p.hasItem && p.itemActive) {
            XMFLOAT3 itemPos = XMFLOAT3(p.pos.x, p.pos.y + p.scale.y * 0.5f + 0.6f, p.pos.z);
            float dx = g_playerPos.x - itemPos.x;
            float dy = g_playerPos.y - itemPos.y;
            float dz = g_playerPos.z - itemPos.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < 1.4f * 1.4f) {
                p.itemActive = false;
                if (p.itemType == ITEM_HIGH_JUMP) {
                    g_highJumpTimer = g_highJumpDuration;
                    PlayMidiNote(0, 84, 127);
                }
                else if (p.itemType == ITEM_FLY) {
                    g_flyTimer = g_flyDuration;
                    PlayMidiNote(0, 96, 127);
                }
                else if (p.itemType == ITEM_GUN) {
                    g_ammoCount = 5;
                    PlayMidiNote(0, 91, 127);
                }
            }
        }
    }

    XMFLOAT3 moveDir = { 0.0f, 0.0f, 0.0f };

    if (g_stunTimer <= 0.0f) {
        if (GetAsyncKeyState('W') & 0x8000) { moveDir.z += 1.0f; }
        if (GetAsyncKeyState('S') & 0x8000) { moveDir.z -= 1.0f; }
        if (GetAsyncKeyState('A') & 0x8000) { moveDir.x -= 1.0f; }
        if (GetAsyncKeyState('D') & 0x8000) { moveDir.x += 1.0f; }
    }

    float forwardX = sinf(g_camYaw);
    float forwardZ = cosf(g_camYaw);
    float rightX = cosf(g_camYaw);
    float rightZ = -sinf(g_camYaw);

    XMFLOAT3 wishVel;
    wishVel.x = (forwardX * moveDir.z + rightX * moveDir.x) * g_moveSpeed;
    wishVel.z = (forwardZ * moveDir.z + rightZ * moveDir.x) * g_moveSpeed;

    g_playerPos.x += wishVel.x * dt;
    g_playerPos.z += wishVel.z * dt;

    if (g_flyTimer > 0.0f) {
        g_playerVel.y = g_flySpeed;
        g_playerPos.z += (g_moveSpeed + 1.0f) * dt;
        g_playerGrounded = false;
    }
    else {
        g_playerVel.y -= 22.0f * dt;
    }

    g_playerPos.y += g_playerVel.y * dt;

    g_playerGrounded = false;

    for (auto& p : g_platforms) {
        if (p.isDestroyed) continue;

        bool overlapX = (g_playerPos.x + 0.38f >= p.pos.x - p.scale.x * 0.5f) && (g_playerPos.x - 0.38f <= p.pos.x + p.scale.x * 0.5f);
        bool overlapZ = (g_playerPos.z + 0.38f >= p.pos.z - p.scale.z * 0.5f) && (g_playerPos.z - 0.38f <= p.pos.z + p.scale.z * 0.5f);

        if (overlapX && overlapZ) {
            float platTop = p.pos.y + p.scale.y * 0.5f;
            float playerBottom = g_playerPos.y - 0.8f;

            if (playerBottom >= platTop - 0.65f && playerBottom <= platTop + 0.35f && g_playerVel.y <= 1.5f) {
                g_playerPos.y = platTop + 0.8f;
                g_playerVel.y = 0.0f;
                g_playerGrounded = true;

                if (p.isCracked && !p.isCracking) {
                    p.isCracking = true;
                    p.crackTimer = 0.35f;
                    PlayMidiNote(9, 37, 90);
                }

                if (p.moveAxis == 1) {
                    g_playerPos.x += cosf(g_sceneTime * p.moveSpeed) * p.moveAmp * p.moveSpeed * dt;
                }

                if (p.type == PLATFORM_BOOST) {
                    g_playerVel.y = g_highJumpPower + 0.5f;
                    g_playerGrounded = false;
                    PlayMidiNote(9, 72, 120);
                }
                break;
            }
        }
    }

    if (g_playerGrounded) {
        g_timeSinceGrounded = 0.0f;
    }
    else {
        g_timeSinceGrounded += dt;
    }

    if (g_stunTimer <= 0.0f && (g_playerGrounded || g_timeSinceGrounded < 0.15f) && (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
        float jumpPower = (g_highJumpTimer > 0.0f) ? g_highJumpPower : g_normalJumpPower;
        g_playerVel.y = jumpPower;
        g_playerGrounded = false;
        g_timeSinceGrounded = 0.3f;
        PlayMidiNote(9, (g_highJumpTimer > 0.0f) ? 75 : 60, 110);
    }

    if ((int)g_playerPos.z > g_jumpScore) {
        g_jumpScore = (int)g_playerPos.z;
        
    }

    if (g_playerPos.z + 40.0f > g_lastSpawnZ) {
        g_lastSpawnZ += 5.0f + (rand() % 100) / 50.0f;
        SpawnJumpPlatformCluster(g_lastSpawnZ);
    }

    if (g_playerPos.y < -12.0f) {
        g_isJumpGameOver = true;
        PlayMidiNote(0, 36, 120);
    }
}

void AdjustSelectedParameter(int paramIdx, float direction) {
    TuningParam params[PARAM_COUNT] = {
        { L"1. Move Speed",          &g_moveSpeed,          1.0f,  20.0f, 1.0f, L"" },
        { L"2. Normal Jump Power",   &g_normalJumpPower,    1.0f,  25.0f, 1.0f, L"" },
        { L"3. Boost Jump Power",    &g_highJumpPower,      1.0f,  35.0f, 1.0f, L"" },
        { L"4. Jetpack Fly Speed",   &g_flySpeed,           1.0f,  20.0f, 0.5f, L"" },
        { L"5. Jump Buff Duration",  &g_highJumpDuration,   0.5f,  20.0f, 0.5f, L"s" },
        { L"6. Fly Buff Duration",   &g_flyDuration,        0.5f,  20.0f, 0.5f, L"s" },
        { L"7. Bullet Speed",        &g_bulletSpeed,        10.0f, 200.0f,5.0f, L"" },
        { L"8. Enemy Shoot Interval",&g_enemyShootInterval, 0.5f,  10.0f, 0.2f, L"s" },
        { L"9. Stun Duration",       &g_stunDuration,       0.0f,  5.0f,  0.2f, L"s" }
    };

    if (paramIdx >= 0 && paramIdx < PARAM_COUNT) {
        float newVal = *(params[paramIdx].pValue) + direction * params[paramIdx].step;
        if (newVal < params[paramIdx].minVal) newVal = params[paramIdx].minVal;
        if (newVal > params[paramIdx].maxVal) newVal = params[paramIdx].maxVal;
        *(params[paramIdx].pValue) = newVal;
    }
}

void HandleMouseClick(int x, int y) {
    if (g_gameState == STATE_TITLE) {
        if (x >= 362 && x <= 662 && y >= 480 && y <= 540) {
            InitJumpGame();
            g_gameState = STATE_GAMEPLAY;
            PlayMidiNote(0, 72, 100);
        }
        else if (x >= 362 && x <= 662 && y >= 570 && y <= 630) {
            g_gameState = STATE_TUNING;
            PlayMidiNote(0, 80, 100);
        }
        else if (x >= 362 && x <= 662 && y >= 660 && y <= 720) {
            PlayMidiNote(0, 50, 100);
            isForceEndRelay = true; // ⭐ 릴레이 강제 종료 신호 ON
            gameovercheck = true;
        }
    }
    else if (g_gameState == STATE_TUNING) {
        if (x >= 362 && x <= 662 && y >= 665 && y <= 715) {
            g_gameState = STATE_TITLE;
            PlayMidiNote(0, 70, 100);
            return;
        }

        for (int i = 0; i < PARAM_COUNT; ++i) {
            int rowY = 145 + i * 48;
            if (x >= 680 && x <= 730 && y >= rowY && y <= rowY + 36) {
                AdjustSelectedParameter(i, -1.0f);
                g_selectedParam = i;
                PlayMidiNote(0, 75, 90);
                return;
            }
            if (x >= 740 && x <= 790 && y >= rowY && y <= rowY + 36) {
                AdjustSelectedParameter(i, 1.0f);
                g_selectedParam = i;
                PlayMidiNote(0, 85, 90);
                return;
            }
            if (x >= 105 && x <= 670 && y >= rowY && y <= rowY + 36) {
                g_selectedParam = i;
                PlayMidiNote(0, 80, 80);
                return;
            }
        }
    }
}

void ProcessInput(WPARAM key) {
    if (g_gameState == STATE_TITLE) {
        if (key == VK_RETURN) {
            InitJumpGame();
            g_gameState = STATE_GAMEPLAY;
            PlayMidiNote(0, 72, 100);
        }
        return;
    }

    if (g_gameState == STATE_TUNING) {
        if (key == VK_ESCAPE) {
            g_gameState = STATE_TITLE;
            return;
        }
        if (key == VK_UP) {
            g_selectedParam = (g_selectedParam - 1 + PARAM_COUNT) % PARAM_COUNT;
            PlayMidiNote(0, 80, 80);
            return;
        }
        if (key == VK_DOWN) {
            g_selectedParam = (g_selectedParam + 1) % PARAM_COUNT;
            PlayMidiNote(0, 80, 80);
            return;
        }
        if (key == VK_LEFT) {
            AdjustSelectedParameter(g_selectedParam, -1.0f);
            PlayMidiNote(0, 75, 90);
            return;
        }
        if (key == VK_RIGHT) {
            AdjustSelectedParameter(g_selectedParam, 1.0f);
            PlayMidiNote(0, 85, 90);
            return;
        }
    }

    if (g_gameState == STATE_GAMEPLAY) {
        if (key == VK_ESCAPE) {
            g_gameState = STATE_TITLE;
            return;
        }
        if (g_isJumpGameOver && (key == VK_SPACE || key == VK_RETURN)) {
            gameovercheck = g_isJumpGameOver;
        }
    }
}

void UpdateUIOverlay() {
    if (!g_pUIBits) return;
    memset(g_pUIBits, 0, 1024 * 768 * 4);
    SetBkMode(g_hMemDC, TRANSPARENT);

    POINT mousePt;
    GetCursorPos(&mousePt);
    ScreenToClient(g_hWnd, &mousePt);
    int mx = mousePt.x;
    int my = mousePt.y;

    if (g_gameState == STATE_TITLE) {
        SelectObject(g_hMemDC, g_hFontTitle); SetTextColor(g_hMemDC, RGB(255, 215, 0));
        RECT rTitle = { 0, 60, 1024, 150 }; DrawTextW(g_hMemDC, L"INFINITE ROBOT JUMP", -1, &rTitle, DT_CENTER | DT_SINGLELINE);
        bool hoverStart = (mx >= 362 && mx <= 662 && my >= 480 && my <= 540);
        HBRUSH hStartBg = CreateSolidBrush(hoverStart ? RGB(40, 100, 180) : RGB(25, 45, 75));
        HPEN hStartPen = CreatePen(PS_SOLID, 3, hoverStart ? RGB(255, 230, 0) : RGB(0, 180, 255));
        SelectObject(g_hMemDC, hStartBg); SelectObject(g_hMemDC, hStartPen); RoundRect(g_hMemDC, 362, 480, 662, 540, 15, 15);
        DeleteObject(hStartBg); DeleteObject(hStartPen);
        SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, hoverStart ? RGB(255, 255, 255) : RGB(200, 230, 255));
        RECT rStartTxt = { 362, 480, 662, 540 }; DrawTextW(g_hMemDC, L"START GAME", -1, &rStartTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        // =========================================================
        // 2번째 버튼: PARAM TUNING MENU (원래 주황/갈색 톤)
        // =========================================================
        bool hoverTune = (mx >= 362 && mx <= 662 && my >= 570 && my <= 630);
        HBRUSH hTuneBg = CreateSolidBrush(hoverTune ? RGB(160, 90, 20) : RGB(65, 40, 20));
        HPEN hTunePen = CreatePen(PS_SOLID, 3, hoverTune ? RGB(255, 230, 0) : RGB(255, 140, 0));
        SelectObject(g_hMemDC, hTuneBg); SelectObject(g_hMemDC, hTunePen);
        RoundRect(g_hMemDC, 362, 570, 662, 630, 15, 15);
        DeleteObject(hTuneBg); DeleteObject(hTunePen);

        SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, hoverTune ? RGB(255, 255, 255) : RGB(255, 210, 160));
        RECT rTuneTxt = { 362, 570, 662, 630 }; DrawTextW(g_hMemDC, L"PARAM TUNING MENU", -1, &rTuneTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // =========================================================
        // 3번째 버튼: END RELAY (원래 EXIT GAME의 붉은색 톤)
        // =========================================================
        bool hoverExit = (mx >= 362 && mx <= 662 && my >= 660 && my <= 720);
        HBRUSH hExitBg = CreateSolidBrush(hoverExit ? RGB(180, 40, 40) : RGB(70, 25, 25));
        HPEN hExitPen = CreatePen(PS_SOLID, 3, hoverExit ? RGB(255, 230, 0) : RGB(255, 80, 80));
        SelectObject(g_hMemDC, hExitBg); SelectObject(g_hMemDC, hExitPen);
        RoundRect(g_hMemDC, 362, 660, 662, 720, 15, 15);
        DeleteObject(hExitBg); DeleteObject(hExitPen);

        SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, hoverExit ? RGB(255, 255, 255) : RGB(255, 180, 180));
        RECT rExitTxt = { 362, 660, 662, 720 }; DrawTextW(g_hMemDC, L"END RELAY", -1, &rExitTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 조작법
        SetTextColor(g_hMemDC, RGB(255, 255, 0));
        RECT rCtrl = { 0, 400, 1024, 700 }; DrawTextW(g_hMemDC, L"[WASD] MOVE | [SPACE] JUMP | [LMB] SHOOT", -1, &rCtrl, DT_CENTER | DT_SINGLELINE);
    
     
    }
    else if (g_gameState == STATE_TUNING) {
        HBRUSH hMenuBg = CreateSolidBrush(RGB(18, 22, 32));
        HPEN hMenuPen = CreatePen(PS_SOLID, 2, RGB(0, 200, 255));
        SelectObject(g_hMemDC, hMenuBg); SelectObject(g_hMemDC, hMenuPen);
        RoundRect(g_hMemDC, 80, 60, 944, 650, 20, 20);
        DeleteObject(hMenuBg); DeleteObject(hMenuPen);

        SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, RGB(0, 220, 255));
        RECT rTuningTitle = { 110, 75, 910, 105 };
        DrawTextW(g_hMemDC, L"[ GAME PARAMETER TUNING MENU ]", -1, &rTuningTitle, DT_LEFT | DT_SINGLELINE);

        SelectObject(g_hMemDC, g_hFontSub); SetTextColor(g_hMemDC, RGB(180, 200, 220));
        RECT rSubHint = { 110, 108, 910, 130 };
        DrawTextW(g_hMemDC, L"Click [-] / [+] buttons or use Arrow Keys to adjust values dynamically.", -1, &rSubHint, DT_LEFT | DT_SINGLELINE);

        TuningParam params[PARAM_COUNT] = {
            { L"1. Move Speed",          &g_moveSpeed,          1.0f,  20.0f, 1.0f, L"" },
            { L"2. Normal Jump Power",   &g_normalJumpPower,    1.0f,  25.0f, 1.0f, L"" },
            { L"3. Boost Jump Power",    &g_highJumpPower,      1.0f,  35.0f, 1.0f, L"" },
            { L"4. Jetpack Fly Speed",   &g_flySpeed,           1.0f,  20.0f, 0.5f, L"" },
            { L"5. Jump Buff Duration",  &g_highJumpDuration,   0.5f,  20.0f, 0.5f, L"s" },
            { L"6. Fly Buff Duration",   &g_flyDuration,        0.5f,  20.0f, 0.5f, L"s" },
            { L"7. Bullet Speed",        &g_bulletSpeed,        10.0f, 200.0f,5.0f, L"" },
            { L"8. Enemy Shoot Interval",&g_enemyShootInterval, 0.5f,  10.0f, 0.2f, L"s" },
            { L"9. Stun Duration",       &g_stunDuration,       0.0f,  5.0f,  0.2f, L"s" }
        };

        for (int i = 0; i < PARAM_COUNT; ++i) {
            int rowY = 145 + i * 48;
            bool isSelected = (i == g_selectedParam);

            if (isSelected) {
                HBRUSH hRowBg = CreateSolidBrush(RGB(35, 50, 75));
                HPEN hRowPen = CreatePen(PS_SOLID, 1, RGB(0, 180, 255));
                SelectObject(g_hMemDC, hRowBg); SelectObject(g_hMemDC, hRowPen);
                RoundRect(g_hMemDC, 105, rowY - 3, 800, rowY + 39, 8, 8);
                DeleteObject(hRowBg); DeleteObject(hRowPen);
            }

            SelectObject(g_hMemDC, g_hFontSub);
            SetTextColor(g_hMemDC, isSelected ? RGB(255, 230, 0) : RGB(210, 210, 210));
            RECT rName = { 115, rowY + 8, 380, rowY + 32 };
            DrawTextW(g_hMemDC, params[i].name, -1, &rName, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            HBRUSH hBarBg = CreateSolidBrush(RGB(40, 45, 55));
            HPEN hBarPen = CreatePen(PS_SOLID, 1, RGB(70, 80, 100));
            SelectObject(g_hMemDC, hBarBg); SelectObject(g_hMemDC, hBarPen);
            RoundRect(g_hMemDC, 380, rowY + 8, 550, rowY + 28, 6, 6);
            DeleteObject(hBarBg); DeleteObject(hBarPen);

            float curVal = *(params[i].pValue);
            float ratio = (curVal - params[i].minVal) / (params[i].maxVal - params[i].minVal);
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;

            int fillWidth = (int)(ratio * (550 - 380 - 4));
            if (fillWidth > 0) {
                COLORREF barColor = isSelected ? RGB(255, 180, 0) : RGB(0, 200, 220);
                HBRUSH hBarFill = CreateSolidBrush(barColor);
                SelectObject(g_hMemDC, hBarFill);
                RoundRect(g_hMemDC, 382, rowY + 10, 382 + fillWidth, rowY + 26, 4, 4);
                DeleteObject(hBarFill);
            }

            HBRUSH hValBg = CreateSolidBrush(RGB(25, 30, 42));
            HPEN hValPen = CreatePen(PS_SOLID, 1, isSelected ? RGB(255, 200, 0) : RGB(100, 120, 150));
            SelectObject(g_hMemDC, hValBg); SelectObject(g_hMemDC, hValPen);
            RoundRect(g_hMemDC, 560, rowY + 4, 660, rowY + 32, 6, 6);
            DeleteObject(hValBg); DeleteObject(hValPen);

            wchar_t valStr[32];
            swprintf_s(valStr, 32, L"%.1f %s", curVal, params[i].unit);

            SelectObject(g_hMemDC, g_hFontSub);
            SetTextColor(g_hMemDC, isSelected ? RGB(255, 230, 0) : RGB(100, 255, 180));
            RECT rVal = { 560, rowY + 4, 660, rowY + 32 };
            DrawTextW(g_hMemDC, valStr, -1, &rVal, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            bool hoverMinus = (mx >= 680 && mx <= 730 && my >= rowY && my <= rowY + 36);
            HBRUSH hMinusBg = CreateSolidBrush(hoverMinus ? RGB(180, 50, 50) : RGB(80, 30, 30));
            HPEN hBtnPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            SelectObject(g_hMemDC, hMinusBg); SelectObject(g_hMemDC, hBtnPen);
            RoundRect(g_hMemDC, 680, rowY, 730, rowY + 36, 8, 8);
            DeleteObject(hMinusBg); DeleteObject(hBtnPen);

            SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, RGB(255, 255, 255));
            RECT rMinusTxt = { 680, rowY, 730, rowY + 36 }; DrawTextW(g_hMemDC, L"-", -1, &rMinusTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            bool hoverPlus = (mx >= 740 && mx <= 790 && my >= rowY && my <= rowY + 36);
            HBRUSH hPlusBg = CreateSolidBrush(hoverPlus ? RGB(40, 160, 60) : RGB(20, 80, 30));
            hBtnPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            SelectObject(g_hMemDC, hPlusBg); SelectObject(g_hMemDC, hBtnPen);
            RoundRect(g_hMemDC, 740, rowY, 790, rowY + 36, 8, 8);
            DeleteObject(hPlusBg); DeleteObject(hBtnPen);

            SetTextColor(g_hMemDC, RGB(255, 255, 255));
            RECT rPlusTxt = { 740, rowY, 790, rowY + 36 }; DrawTextW(g_hMemDC, L"+", -1, &rPlusTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        bool hoverBack = (mx >= 362 && mx <= 662 && my >= 665 && my <= 715);
        HBRUSH hBackBg = CreateSolidBrush(hoverBack ? RGB(100, 100, 120) : RGB(40, 45, 60));
        HPEN hBackPen = CreatePen(PS_SOLID, 2, hoverBack ? RGB(255, 230, 0) : RGB(180, 180, 200));
        SelectObject(g_hMemDC, hBackBg); SelectObject(g_hMemDC, hBackPen);
        RoundRect(g_hMemDC, 362, 665, 662, 715, 12, 12);
        DeleteObject(hBackBg); DeleteObject(hBackPen);

        SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, hoverBack ? RGB(255, 255, 255) : RGB(220, 220, 220));
        RECT rBackTxt = { 362, 665, 662, 715 }; DrawTextW(g_hMemDC, L"RETURN TO TITLE", -1, &rBackTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    else if (g_gameState == STATE_GAMEPLAY) {
        if (g_isFpsAiming) {
            HPEN hAimPen = CreatePen(PS_SOLID, 2, RGB(255, 30, 30));
            HPEN hOldPen = (HPEN)SelectObject(g_hMemDC, hAimPen);
            HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(g_hMemDC, hNullBrush);

            int cx = 512, cy = 384;
            Ellipse(g_hMemDC, cx - 18, cy - 18, cx + 18, cy + 18);
            Ellipse(g_hMemDC, cx - 4, cy - 4, cx + 4, cy + 4);
            MoveToEx(g_hMemDC, cx - 28, cy, NULL); LineTo(g_hMemDC, cx - 8, cy);
            MoveToEx(g_hMemDC, cx + 8, cy, NULL);  LineTo(g_hMemDC, cx + 28, cy);
            MoveToEx(g_hMemDC, cx, cy - 28, NULL); LineTo(g_hMemDC, cx, cy - 8);
            MoveToEx(g_hMemDC, cx, cy + 8, NULL);  LineTo(g_hMemDC, cx, cy + 28);

            SelectObject(g_hMemDC, hOldPen); SelectObject(g_hMemDC, hOldBrush); DeleteObject(hAimPen);

            SelectObject(g_hMemDC, g_hFontSub); SetTextColor(g_hMemDC, RGB(255, 50, 50));
            RECT rAimTxt = { 0, 420, 1024, 450 }; DrawTextW(g_hMemDC, L"[ 1ST PERSON FPS AIM MODE ]", -1, &rAimTxt, DT_CENTER | DT_SINGLELINE);
        }
        else if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
            HPEN hAimPen = CreatePen(PS_SOLID, 2, RGB(255, 200, 50));
            HPEN hOldPen = (HPEN)SelectObject(g_hMemDC, hAimPen);
            int cx = 512, cy = 384;
            MoveToEx(g_hMemDC, cx - 15, cy, NULL); LineTo(g_hMemDC, cx + 15, cy);
            MoveToEx(g_hMemDC, cx, cy - 15, NULL); LineTo(g_hMemDC, cx, cy + 15);
            SelectObject(g_hMemDC, hOldPen); DeleteObject(hAimPen);
        }

        SelectObject(g_hMemDC, g_hFontBig); SetTextColor(g_hMemDC, RGB(255, 255, 255));
        wchar_t scoreBuf[64]; swprintf_s(scoreBuf, 64, L"DISTANCE : %d m", g_jumpScore);
        RECT rScore = { 40, 30, 400, 80 }; DrawTextW(g_hMemDC, scoreBuf, -1, &rScore, DT_LEFT | DT_SINGLELINE);

       

        if (g_stunTimer > 0.0f) {
            SelectObject(g_hMemDC, g_hFontTitle); SetTextColor(g_hMemDC, RGB(255, 220, 0));
            wchar_t stunBuf[64]; swprintf_s(stunBuf, 64, L"STUNNED! (%.1fs)", g_stunTimer);
            RECT rStun = { 0, 180, 1024, 250 }; DrawTextW(g_hMemDC, stunBuf, -1, &rStun, DT_CENTER | DT_SINGLELINE);
        }

        if (g_ammoCount > 0) {
            SelectObject(g_hMemDC, g_hFontBig); SetTextColor(g_hMemDC, RGB(180, 80, 255));
            wchar_t ammoBuf[64]; swprintf_s(ammoBuf, 64, L"AMMO : %d / 5", g_ammoCount);
            RECT rAmmo = { 40, 75, 400, 115 }; DrawTextW(g_hMemDC, ammoBuf, -1, &rAmmo, DT_LEFT | DT_SINGLELINE);
        }

        if (g_sideEnemy.active) {
            SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, RGB(255, 50, 80));
            wchar_t eBuf[64]; swprintf_s(eBuf, 64, L"ENEMY SPOTTED! HP: [%d / 2]", g_sideEnemy.hp);
            RECT rE = { 700, 85, 980, 125 }; DrawTextW(g_hMemDC, eBuf, -1, &rE, DT_RIGHT | DT_SINGLELINE);
        }

        if (g_highJumpTimer > 0.0f) {
            wchar_t buff1[64]; swprintf_s(buff1, 64, L"JUMP BOOST : %.1fs", g_highJumpTimer);
            SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, RGB(255, 215, 0));
            RECT rBuff1 = { 40, 120, 400, 160 }; DrawTextW(g_hMemDC, buff1, -1, &rBuff1, DT_LEFT | DT_SINGLELINE);
        }

        if (g_flyTimer > 0.0f) {
            wchar_t buff2[64]; swprintf_s(buff2, 64, L"JETPACK FLY : %.1fs", g_flyTimer);
            SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, RGB(0, 220, 255));
            RECT rBuff2 = { 40, 155, 400, 195 }; DrawTextW(g_hMemDC, buff2, -1, &rBuff2, DT_LEFT | DT_SINGLELINE);
        }

        SelectObject(g_hMemDC, g_hFontSub); SetTextColor(g_hMemDC, RGB(200, 220, 255));
        RECT rLegend1 = { 40, 690, 980, 715 }; DrawTextW(g_hMemDC, L"[RMB: Hold for 1st Person FPS Aim] [LMB/F: Shoot] [ESC: Return to Title]", -1, &rLegend1, DT_LEFT | DT_SINGLELINE);

        SetTextColor(g_hMemDC, RGB(220, 220, 220));
        RECT rControls = { 40, 720, 980, 750 }; DrawTextW(g_hMemDC, L"WASD: Move | SPACE: Jump | Adjust game parameters on Title Screen!", -1, &rControls, DT_LEFT | DT_SINGLELINE);

        if (g_isJumpGameOver) {
            HBRUSH hBoxBrush = CreateSolidBrush(RGB(45, 20, 25)); HPEN hBoxPen = CreatePen(PS_SOLID, 3, RGB(220, 60, 60));
            SelectObject(g_hMemDC, hBoxBrush); SelectObject(g_hMemDC, hBoxPen); RoundRect(g_hMemDC, 212, 214, 812, 554, 20, 20);
            DeleteObject(hBoxBrush); DeleteObject(hBoxPen);

            SelectObject(g_hMemDC, g_hFontTitle); SetTextColor(g_hMemDC, RGB(255, 70, 70));
            RECT rOver = { 212, 250, 812, 320 }; DrawTextW(g_hMemDC, L"FALL DOWN!", -1, &rOver, DT_CENTER | DT_SINGLELINE);

            SelectObject(g_hMemDC, g_hFontBig); SetTextColor(g_hMemDC, RGB(255, 255, 255));
            wchar_t finalBuf[64]; swprintf_s(finalBuf, 64, L"FINAL RECORD : %d m", g_jumpScore);
            RECT rInfo = { 212, 340, 812, 400 }; DrawTextW(g_hMemDC, finalBuf, -1, &rInfo, DT_CENTER | DT_SINGLELINE);

            SelectObject(g_hMemDC, g_hFontMed); SetTextColor(g_hMemDC, RGB(200, 200, 200));
            RECT rHint = { 212, 450, 812, 510 }; DrawTextW(g_hMemDC, L"PRESS SPACE TO NEXT GAME", -1, &rHint, DT_CENTER | DT_SINGLELINE);
        }
    }

    DWORD* pPixels = (DWORD*)g_pUIBits;
    for (int i = 0; i < 1024 * 768; ++i) { if (pPixels[i] != 0) pPixels[i] |= 0xFF000000; }
    g_pContext->UpdateSubresource(g_pUITexture, 0, NULL, g_pUIBits, 1024 * 4, 0);
}

void Create16x16IndexTexture() {
    XMFLOAT4 palette[256];
    palette[0] = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    palette[1] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    palette[2] = XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f);
    for (int i = 3; i < 256; ++i) palette[i] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    DWORD pixelData[16 * 16];
    for (int i = 0; i < 256; ++i) {
        BYTE idx = g_indexImage16x16[i];
        XMFLOAT4 col = palette[idx];
        pixelData[i] = ((BYTE)(col.w * 255) << 24) | ((BYTE)(col.z * 255) << 16) | ((BYTE)(col.y * 255) << 8) | (BYTE)(col.x * 255);
    }

    D3D11_TEXTURE2D_DESC desc = { 16, 16, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    D3D11_SUBRESOURCE_DATA initData = { pixelData, 16 * sizeof(DWORD), 0 };
    ID3D11Texture2D* pTex = nullptr;
    g_pDevice->CreateTexture2D(&desc, &initData, &pTex);
    g_pDevice->CreateShaderResourceView(pTex, nullptr, &g_pTextureSRV);
    pTex->Release();

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    g_pDevice->CreateSamplerState(&sampDesc, &g_pSamplerState);
}

bool InitUIResources() {
    HDC hDC = GetDC(NULL); g_hMemDC = CreateCompatibleDC(hDC);
    BITMAPINFO bmi = {}; bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 1024; bmi.bmiHeader.biHeight = -768; bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biCompression = BI_RGB;
    g_hUIBitmap = CreateDIBSection(g_hMemDC, &bmi, DIB_RGB_COLORS, &g_pUIBits, NULL, 0);
    SelectObject(g_hMemDC, g_hUIBitmap); ReleaseDC(NULL, hDC);

    g_hFontTitle = CreateFontW(52, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    g_hFontBig = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    g_hFontMed = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    g_hFontSub = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");

    D3D11_TEXTURE2D_DESC td = { 1024, 768, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    g_pDevice->CreateTexture2D(&td, NULL, &g_pUITexture);
    g_pDevice->CreateShaderResourceView(g_pUITexture, NULL, &g_pUISRV);

    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    g_pDevice->CreateBlendState(&bd, &g_pUIBlendState);

    Vertex uiVertices[] = {
        { XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1024.0f, 0.0f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1024.0f, 768.0f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 768.0f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT2(0.0f, 1.0f) },
    };
    D3D11_BUFFER_DESC vbd = { sizeof(Vertex) * 4, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA vInitData = { uiVertices };
    g_pDevice->CreateBuffer(&vbd, &vInitData, &g_pUIVertexBuffer);

    WORD uiIndices[] = { 0, 1, 2, 0, 2, 3 };
    D3D11_BUFFER_DESC ibd = { sizeof(WORD) * 6, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA iInitData = { uiIndices };
    g_pDevice->CreateBuffer(&ibd, &iInitData, &g_pUIIndexBuffer);

    return true;
}

// ==========================================
// ?? 팀원 통합 요청 반영 표준 라이프사이클 함수들
// ==========================================

void Init(HWND hWnd) {
    g_hWnd = hWnd;
    RECT rc; GetClientRect(hWnd, &rc);
    UINT width = rc.right - rc.left; UINT height = rc.bottom - rc.top;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; sd.BufferDesc.Width = width; sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pDevice, nullptr, &g_pContext))) return;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRTV);
    pBackBuffer->Release();

    D3D11_TEXTURE2D_DESC depthDesc = { width, height, 1, 1, DXGI_FORMAT_D24_UNORM_S8_UINT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_DEPTH_STENCIL, 0, 0 };
    g_pDevice->CreateTexture2D(&depthDesc, nullptr, &g_pDepthBuffer);
    g_pDevice->CreateDepthStencilView(g_pDepthBuffer, nullptr, &g_pDSV);
    g_pContext->OMSetRenderTargets(1, &g_pRTV, g_pDSV);

    D3D11_VIEWPORT vp = { 0.0f, 0.0f, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
    g_pContext->RSSetViewports(1, &vp);

    ID3DBlob* pVSBlob = nullptr; ID3DBlob* pPSBlob = nullptr;
    D3DCompile(g_shaderSource, strlen(g_shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &pVSBlob, nullptr);
    D3DCompile(g_shaderSource, strlen(g_shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &pPSBlob, nullptr);

    g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVS);
    g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPS);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    g_pDevice->CreateInputLayout(layout, 3, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pInputLayout);
    pVSBlob->Release(); pPSBlob->Release();

    Vertex vertices[] = {
        { XMFLOAT3(-0.5f,-0.5f,-0.5f), XMFLOAT3(0,0,-1), XMFLOAT2(0,1) }, { XMFLOAT3(-0.5f,0.5f,-0.5f), XMFLOAT3(0,0,-1), XMFLOAT2(0,0) },
        { XMFLOAT3(0.5f,0.5f,-0.5f), XMFLOAT3(0,0,-1), XMFLOAT2(1,0) }, { XMFLOAT3(0.5f,-0.5f,-0.5f), XMFLOAT3(0,0,-1), XMFLOAT2(1,1) },
        { XMFLOAT3(-0.5f,-0.5f,0.5f), XMFLOAT3(0,0,1), XMFLOAT2(1,1) }, { XMFLOAT3(0.5f,-0.5f,0.5f), XMFLOAT3(0,0,1), XMFLOAT2(0,1) },
        { XMFLOAT3(0.5f,0.5f,0.5f), XMFLOAT3(0,0,1), XMFLOAT2(0,0) }, { XMFLOAT3(-0.5f,0.5f,0.5f), XMFLOAT3(0,0,1), XMFLOAT2(1,0) },
        { XMFLOAT3(-0.5f,0.5f,-0.5f), XMFLOAT3(0,1,0), XMFLOAT2(0,1) }, { XMFLOAT3(-0.5f,0.5f,0.5f), XMFLOAT3(0,1,0), XMFLOAT2(0,0) },
        { XMFLOAT3(0.5f,0.5f,0.5f), XMFLOAT3(0,1,0), XMFLOAT2(1,0) }, { XMFLOAT3(0.5f,0.5f,-0.5f), XMFLOAT3(0,1,0), XMFLOAT2(1,1) },
        { XMFLOAT3(-0.5f,-0.5f,-0.5f), XMFLOAT3(0,-1,0), XMFLOAT2(0,0) }, { XMFLOAT3(0.5f,-0.5f,-0.5f), XMFLOAT3(0,-1,0), XMFLOAT2(1,0) },
        { XMFLOAT3(0.5f,-0.5f,0.5f), XMFLOAT3(0,-1,0), XMFLOAT2(1,1) }, { XMFLOAT3(-0.5f,-0.5f,0.5f), XMFLOAT3(0,-1,0), XMFLOAT2(0,1) },
        { XMFLOAT3(-0.5f,-0.5f,0.5f), XMFLOAT3(-1,0,0), XMFLOAT2(0,1) }, { XMFLOAT3(-0.5f,0.5f,0.5f), XMFLOAT3(-1,0,0), XMFLOAT2(0,0) },
        { XMFLOAT3(-0.5f,0.5f,-0.5f), XMFLOAT3(-1,0,0), XMFLOAT2(1,0) }, { XMFLOAT3(-0.5f,-0.5f,-0.5f), XMFLOAT3(-1,0,0), XMFLOAT2(1,1) },
        { XMFLOAT3(0.5f,-0.5f,-0.5f), XMFLOAT3(1,0,0), XMFLOAT2(0,1) }, { XMFLOAT3(0.5f,0.5f,-0.5f), XMFLOAT3(1,0,0), XMFLOAT2(0,0) },
        { XMFLOAT3(0.5f,0.5f,0.5f), XMFLOAT3(1,0,0), XMFLOAT2(1,0) }, { XMFLOAT3(0.5f,-0.5f,0.5f), XMFLOAT3(1,0,0), XMFLOAT2(1,1) }
    };
    D3D11_BUFFER_DESC vbd = { sizeof(Vertex) * 24, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA vInitData = { vertices };
    g_pDevice->CreateBuffer(&vbd, &vInitData, &g_pVertexBuffer);

    WORD indices[] = { 0,1,2,0,2,3, 4,5,6,4,6,7, 8,9,10,8,10,11, 12,13,14,12,14,15, 16,17,18,16,18,19, 20,21,22,20,22,23 };
    D3D11_BUFFER_DESC ibd = { sizeof(WORD) * 36, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA iInitData = { indices };
    g_pDevice->CreateBuffer(&ibd, &iInitData, &g_pIndexBuffer);

    D3D11_BUFFER_DESC cbd = { sizeof(ConstantBuffer), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    g_pDevice->CreateBuffer(&cbd, nullptr, &g_pConstantBuffer);

    Create16x16IndexTexture();
    InitUIResources();
    InitAudio();
    InitTimer();
 /*   InitJumpGame();
    g_gameState = STATE_GAMEPLAY;*/
}

void Update() {
    float dt = GetDeltaTime();
    g_sceneTime += dt;
    UpdateBGM(dt);

    if (g_gameState == STATE_GAMEPLAY && !g_isJumpGameOver) {
        POINT pt; GetCursorPos(&pt); ScreenToClient(g_hWnd, &pt);
        int centerX = 512, centerY = 384;
        float dx = (float)(pt.x - centerX);
        float dy = (float)(pt.y - centerY);

        g_camYaw += dx * 0.003f;
        g_camPitch -= dy * 0.003f;
        if (g_camPitch > 1.2f) g_camPitch = 1.2f;
        if (g_camPitch < -1.2f) g_camPitch = -1.2f;

        POINT centerPt = { centerX, centerY };
        ClientToScreen(g_hWnd, &centerPt);
        SetCursorPos(centerPt.x, centerPt.y);
        ShowCursor(FALSE);

        UpdateJumpGame(dt);
    }
    else {
        ShowCursor(TRUE);
    }
}

void DrawCube(const XMMATRIX& world, const XMMATRIX& viewProj, XMFLOAT4 color) {
    ConstantBuffer cb;
    cb.World = XMMatrixTranspose(world);
    cb.WVP = XMMatrixTranspose(world * viewProj);
    cb.Color = color;

    g_pContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    g_pContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pContext->PSSetShaderResources(0, 1, &g_pTextureSRV);
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);
    g_pContext->DrawIndexed(36, 0, 0);
}

void DrawUIQuad() {
    UpdateUIOverlay();
    UINT stride = sizeof(Vertex); UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pUIVertexBuffer, &stride, &offset);
    g_pContext->IASetIndexBuffer(g_pUIIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    ConstantBuffer cb;
    cb.World = XMMatrixIdentity();
    cb.WVP = XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(0.0f, 1024.0f, 768.0f, 0.0f, 0.0f, 1.0f));
    cb.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    g_pContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    g_pContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
    g_pContext->PSSetShaderResources(0, 1, &g_pUISRV);
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);

    g_pContext->OMSetBlendState(g_pUIBlendState, NULL, 0xFFFFFFFF);
    g_pContext->DrawIndexed(6, 0, 0);
    g_pContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
}

void Draw() {
    UINT stride = sizeof(Vertex); UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pContext->IASetInputLayout(g_pInputLayout);
    g_pContext->VSSetShader(g_pVS, nullptr, 0);
    g_pContext->PSSetShader(g_pPS, nullptr, 0);

    float clearColor[4] = { 0.15f, 0.2f, 0.3f, 1.0f };
    g_pContext->ClearRenderTargetView(g_pRTV, clearColor);
    g_pContext->ClearDepthStencilView(g_pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    if (g_gameState == STATE_TITLE || g_gameState == STATE_TUNING) {
        XMVECTOR eye = XMVectorSet(0.0f, 1.0f, -4.5f, 0.0f);
        XMVECTOR at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMMATRIX view = XMMatrixLookAtLH(eye, at, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), 1024.0f / 768.0f, 0.1f, 100.0f);
        XMMATRIX viewProj = view * proj;

        float rotY = g_sceneTime * 0.8f;
        float floatY = sinf(g_sceneTime * 3.0f) * 0.08f;

        auto RenderTitleRobotPart = [&](XMMATRIX scale, XMFLOAT3 localPos, XMFLOAT4 color) {
            XMMATRIX partScale = scale;
            XMMATRIX partTrans = XMMatrixTranslation(localPos.x, localPos.y, localPos.z);
            XMMATRIX rotYMat = XMMatrixRotationY(rotY);
            XMMATRIX baseWorld = XMMatrixTranslation(0.0f, floatY - 0.5f, 0.0f);

            XMMATRIX finalWorld = partScale * partTrans * rotYMat * baseWorld;
            DrawCube(finalWorld, viewProj, color);
            };

        XMFLOAT4 mainColor = XMFLOAT4(0.2f, 0.7f, 0.85f, 1.0f);
        RenderTitleRobotPart(XMMatrixScaling(0.48f, 0.45f, 0.45f), XMFLOAT3(0.0f, 0.45f, 0.0f), XMFLOAT4(0.75f, 0.8f, 0.85f, 1.0f));
        RenderTitleRobotPart(XMMatrixScaling(0.6f, 0.6f, 0.4f), XMFLOAT3(0.0f, -0.05f, 0.0f), mainColor);
        RenderTitleRobotPart(XMMatrixScaling(0.16f, 0.5f, 0.16f), XMFLOAT3(-0.42f, -0.05f, 0.0f), XMFLOAT4(0.4f, 0.5f, 0.6f, 1.0f));
        RenderTitleRobotPart(XMMatrixScaling(0.16f, 0.5f, 0.16f), XMFLOAT3(0.42f, -0.05f, 0.0f), XMFLOAT4(0.4f, 0.5f, 0.6f, 1.0f));
        RenderTitleRobotPart(XMMatrixScaling(0.2f, 0.5f, 0.2f), XMFLOAT3(-0.18f, -0.55f, 0.0f), XMFLOAT4(0.4f, 0.5f, 0.6f, 1.0f));
        RenderTitleRobotPart(XMMatrixScaling(0.2f, 0.5f, 0.2f), XMFLOAT3(0.18f, -0.55f, 0.0f), XMFLOAT4(0.4f, 0.5f, 0.6f, 1.0f));
    }
    else if (g_gameState == STATE_GAMEPLAY) {
        XMFLOAT3 shakeOffset = { 0.0f, 0.0f, 0.0f };
        if (g_camShakeTimer > 0.0f) {
            float factor = g_camShakeTimer / 0.35f;
            shakeOffset.x = ((rand() % 100) / 100.0f - 0.5f) * g_camShakeIntensity * factor;
            shakeOffset.y = ((rand() % 100) / 100.0f - 0.5f) * g_camShakeIntensity * factor;
            shakeOffset.z = ((rand() % 100) / 100.0f - 0.5f) * g_camShakeIntensity * factor;
        }

        XMVECTOR camDir = XMVectorSet(cosf(g_camPitch) * sinf(g_camYaw), sinf(g_camPitch), cosf(g_camPitch) * cosf(g_camYaw), 0.0f);
        XMVECTOR pPos = XMVectorSet(g_playerPos.x, g_playerPos.y, g_playerPos.z, 0.0f);

        XMVECTOR eye, at;
        if (g_isFpsAiming) {
            eye = pPos + XMVectorSet(shakeOffset.x, 0.5f + shakeOffset.y, shakeOffset.z, 0.0f);
            at = eye + camDir * 10.0f;
        }
        else {
            float camDist = 6.0f;
            eye = pPos - camDir * camDist + XMVectorSet(shakeOffset.x, 1.5f + shakeOffset.y, shakeOffset.z, 0.0f);
            at = pPos + XMVectorSet(shakeOffset.x, 0.5f + shakeOffset.y, shakeOffset.z, 0.0f);
        }

        XMMATRIX view = XMMatrixLookAtLH(eye, at, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(g_isFpsAiming ? 38.0f : 55.0f), 1024.0f / 768.0f, 0.1f, 200.0f);
        XMMATRIX viewProj = view * proj;

        for (const auto& p : g_platforms) {
            if (!p.isDestroyed) {
                XMFLOAT4 drawColor = p.color;

                if (p.isWarning) {
                    float flash = (sinf(g_sceneTime * 30.0f) + 1.0f) * 0.5f;
                    drawColor = XMFLOAT4(1.0f, p.color.y * (1.0f - flash), p.color.z * (1.0f - flash), 1.0f);
                }

                float dropY = 0.0f;
                float shakeX = 0.0f;
                if (p.isCracking) {
                    float progress = 1.0f - (p.crackTimer / 0.35f);
                    dropY = progress * 0.6f;
                    shakeX = sinf(g_sceneTime * 50.0f) * 0.12f;
                }

                XMMATRIX w = XMMatrixScaling(p.scale.x, p.scale.y, p.scale.z) * XMMatrixTranslation(p.pos.x + shakeX, p.pos.y - dropY, p.pos.z);
                DrawCube(w, viewProj, drawColor);

                if (p.isCracked) {
                    XMFLOAT4 crackColor = XMFLOAT4(0.06f, 0.06f, 0.08f, 1.0f);
                    if (p.isCracking) {
                        float glow = (sinf(g_sceneTime * 40.0f) + 1.0f) * 0.5f;
                        crackColor = XMFLOAT4(1.0f, 0.3f * glow, 0.0f, 1.0f);
                    }

                    struct CrackSeg { float ox, oz, len, angle, width; };
                    static const CrackSeg crackPattern[] = {
                        { 0.0f, 0.0f, 1.3f, 0.3f, 0.06f }, { 0.0f, 0.0f, 1.1f, 1.9f, 0.05f },
                        { 0.0f, 0.0f, 1.2f, -1.2f, 0.06f }, { 0.0f, 0.0f, 0.9f, 2.8f, 0.05f },
                        { 0.35f, 0.12f, 0.65f, 0.85f, 0.04f }, { -0.25f, -0.2f, 0.55f, -0.45f, 0.04f }
                    };

                    for (const auto& cs : crackPattern) {
                        XMMATRIX cScale = XMMatrixScaling(cs.len, 0.04f, cs.width);
                        XMMATRIX cRot = XMMatrixRotationY(cs.angle);
                        XMMATRIX cTrans = XMMatrixTranslation(p.pos.x + cs.ox + shakeX, p.pos.y - dropY + p.scale.y * 0.5f + 0.02f, p.pos.z + cs.oz);
                        DrawCube(cScale * cRot * cTrans, viewProj, crackColor);
                    }
                }

                if (p.isWarning) {
                    float startDistZ = 40.0f;
                    float startDistY = 15.0f;
                    float arcPeak = 10.0f;

                    for (int step = 0; step <= 10; ++step) {
                        float tGuide = step / 10.0f;
                        float gZ = p.pos.z + (1.0f - tGuide) * startDistZ;
                        float gY = p.pos.y + (1.0f - tGuide) * startDistY + sinf(tGuide * 3.14159265f) * arcPeak;

                        float pSize = 0.14f + sinf(g_sceneTime * 20.0f + step * 0.5f) * 0.04f;
                        XMMATRIX guideScale = XMMatrixScaling(pSize, pSize, pSize * 1.8f);
                        XMMATRIX guideTrans = XMMatrixTranslation(p.pos.x, gY, gZ);
                        DrawCube(guideScale * guideTrans, viewProj, XMFLOAT4(1.0f, 0.2f, 0.1f, 0.7f));
                    }

                    if (p.warningTimer <= 0.6f) {
                        float t = 1.0f - (p.warningTimer / 0.6f);
                        float missileZ = p.pos.z + (1.0f - t) * startDistZ;
                        float missileY = p.pos.y + (1.0f - t) * startDistY + sinf(t * 3.14159265f) * arcPeak;

                        float dY = -startDistY + 3.14159265f * arcPeak * cosf(t * 3.14159265f);
                        float pitchAngle = atan2f(dY, startDistZ);

                        XMMATRIX mPitchRot = XMMatrixRotationX(pitchAngle);
                        XMMATRIX mWorld = XMMatrixScaling(0.38f, 0.38f, 1.5f) * mPitchRot * XMMatrixTranslation(p.pos.x, missileY, missileZ);
                        DrawCube(mWorld, viewProj, XMFLOAT4(0.35f, 0.38f, 0.42f, 1.0f));
                    }
                }

                if (p.hasItem && p.itemActive) {
                    float hoverY = p.pos.y - dropY + p.scale.y * 0.5f + 0.65f + sinf(g_sceneTime * 3.5f) * 0.12f;
                    XMMATRIX itemBaseRot = XMMatrixRotationY(g_sceneTime * 2.5f);
                    XMMATRIX itemBaseTrans = XMMatrixTranslation(p.pos.x + shakeX, hoverY, p.pos.z);
                    XMMATRIX itemWorld = itemBaseRot * itemBaseTrans;

                    if (p.itemType == ITEM_FLY) {
                        DrawCube(XMMatrixScaling(0.25f, 0.65f, 0.25f) * XMMatrixTranslation(-0.2f, 0.0f, 0.0f) * itemWorld, viewProj, XMFLOAT4(0.1f, 0.85f, 0.95f, 1.0f));
                        DrawCube(XMMatrixScaling(0.25f, 0.65f, 0.25f) * XMMatrixTranslation(0.2f, 0.0f, 0.0f) * itemWorld, viewProj, XMFLOAT4(0.1f, 0.85f, 0.95f, 1.0f));
                        DrawCube(XMMatrixScaling(0.3f, 0.4f, 0.15f) * XMMatrixTranslation(0.0f, 0.05f, -0.05f) * itemWorld, viewProj, XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f));
                        DrawCube(XMMatrixScaling(0.15f, 0.15f, 0.15f) * XMMatrixTranslation(-0.2f, -0.4f, 0.0f) * itemWorld, viewProj, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
                        DrawCube(XMMatrixScaling(0.15f, 0.15f, 0.15f) * XMMatrixTranslation(0.2f, -0.4f, 0.0f) * itemWorld, viewProj, XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
                        DrawCube(XMMatrixScaling(0.12f, 0.22f, 0.12f) * XMMatrixTranslation(-0.2f, -0.55f, 0.0f) * itemWorld, viewProj, XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f));
                        DrawCube(XMMatrixScaling(0.12f, 0.22f, 0.12f) * XMMatrixTranslation(0.2f, -0.55f, 0.0f) * itemWorld, viewProj, XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f));
                    }
                    else if (p.itemType == ITEM_HIGH_JUMP) {
                        XMFLOAT4 spColor = XMFLOAT4(1.0f, 0.85f, 0.0f, 1.0f);
                        DrawCube(XMMatrixScaling(0.55f, 0.08f, 0.55f) * XMMatrixTranslation(0.0f, -0.3f, 0.0f) * itemWorld, viewProj, XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
                        DrawCube(XMMatrixScaling(0.55f, 0.08f, 0.55f) * XMMatrixTranslation(0.0f, 0.3f, 0.0f) * itemWorld, viewProj, XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
                        DrawCube(XMMatrixScaling(0.42f, 0.08f, 0.42f) * XMMatrixTranslation(0.0f, -0.15f, 0.0f) * itemWorld, viewProj, spColor);
                        DrawCube(XMMatrixScaling(0.32f, 0.08f, 0.32f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f) * itemWorld, viewProj, spColor);
                        DrawCube(XMMatrixScaling(0.42f, 0.08f, 0.42f) * XMMatrixTranslation(0.0f, 0.15f, 0.0f) * itemWorld, viewProj, spColor);
                    }
                    else if (p.itemType == ITEM_GUN) {
                        XMFLOAT4 gunColor = XMFLOAT4(0.75f, 0.2f, 1.0f, 1.0f);
                        DrawCube(XMMatrixScaling(0.12f, 0.12f, 0.75f) * XMMatrixTranslation(0.0f, 0.1f, 0.15f) * itemWorld, viewProj, XMFLOAT4(0.25f, 0.25f, 0.3f, 1.0f));
                        DrawCube(XMMatrixScaling(0.2f, 0.22f, 0.45f) * XMMatrixTranslation(0.0f, 0.05f, -0.1f) * itemWorld, viewProj, gunColor);
                        DrawCube(XMMatrixScaling(0.12f, 0.35f, 0.16f) * XMMatrixTranslation(0.0f, -0.18f, -0.2f) * itemWorld, viewProj, XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
                        DrawCube(XMMatrixScaling(0.1f, 0.1f, 0.22f) * XMMatrixTranslation(0.0f, 0.22f, 0.0f) * itemWorld, viewProj, XMFLOAT4(0.1f, 1.0f, 0.8f, 1.0f));
                    }
                }
            }
        }

        for (const auto& p : g_platforms) {
            if (p.isMissileStriking) {
                float progress = 1.0f - (p.missileAnimTimer / 0.6f);

                float coreScale = 1.2f + progress * 4.8f;
                XMFLOAT4 coreColor = (progress < 0.25f) ? XMFLOAT4(1.0f, 1.0f, 0.8f, 1.0f) :
                    (progress < 0.65f) ? XMFLOAT4(1.0f, 0.45f, 0.0f, 0.95f) :
                    XMFLOAT4(0.35f, 0.12f, 0.12f, 0.6f);

                XMMATRIX coreWorld = XMMatrixScaling(coreScale, coreScale, coreScale) *
                    XMMatrixTranslation(p.missileStrikePos.x, p.missileStrikePos.y + 0.5f, p.missileStrikePos.z);
                DrawCube(coreWorld, viewProj, coreColor);

                static const XMFLOAT3 debrisDirs[] = {
                    { 1.0f,  1.2f,  1.0f }, { -1.0f,  1.2f,  1.0f }, { 1.0f,  1.2f, -1.0f }, { -1.0f,  1.2f, -1.0f },
                    { 1.8f,  1.8f,  0.0f }, { -1.8f,  1.8f,  0.0f }, { 0.0f,  1.8f,  1.8f }, {  0.0f,  1.8f, -1.8f },
                    { 2.2f,  0.8f,  1.0f }, { -2.2f,  0.8f, -1.0f }, { 1.0f,  0.8f, -2.2f }, { -1.0f,  0.8f,  2.2f }
                };

                for (const auto& dir : debrisDirs) {
                    float dist = progress * 7.5f;
                    float py = p.missileStrikePos.y + dir.y * dist - 8.5f * progress * progress;
                    float px = p.missileStrikePos.x + dir.x * dist;
                    float pz = p.missileStrikePos.z + dir.z * dist;

                    float pSize = (1.0f - progress) * 0.48f;
                    if (pSize > 0.02f) {
                        XMMATRIX pRot = XMMatrixRotationRollPitchYaw(g_sceneTime * 15.0f, g_sceneTime * 20.0f, 0.0f);
                        XMMATRIX pWorld = XMMatrixScaling(pSize, pSize, pSize) * pRot * XMMatrixTranslation(px, py, pz);
                        XMFLOAT4 pCol = (rand() % 2 == 0) ? XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f) : XMFLOAT4(0.35f, 0.35f, 0.38f, 1.0f);
                        DrawCube(pWorld, viewProj, pCol);
                    }
                }
            }
        }

        if (g_sideEnemy.active) {
            XMFLOAT4 eColor = (g_sideEnemy.hitFlash > 0.0f) ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) : XMFLOAT4(0.8f, 0.1f, 0.2f, 1.0f);

            XMMATRIX bodyScale = XMMatrixScaling(1.4f, 0.6f, 1.4f);
            XMMATRIX bodyRot = XMMatrixRotationY(g_sceneTime * 4.0f);
            XMMATRIX bodyTrans = XMMatrixTranslation(g_sideEnemy.pos.x, g_sideEnemy.pos.y, g_sideEnemy.pos.z);
            DrawCube(bodyScale * bodyRot * bodyTrans, viewProj, eColor);

            XMMATRIX eyeScale = XMMatrixScaling(0.4f, 0.4f, 0.4f);
            XMMATRIX eyeTrans = XMMatrixTranslation(g_sideEnemy.pos.x, g_sideEnemy.pos.y, g_sideEnemy.pos.z - 0.7f);
            DrawCube(eyeScale * eyeTrans, viewProj, XMFLOAT4(1.0f, 0.0f, 0.8f, 1.0f));

            for (int r = 0; r < 4; ++r) {
                float angle = g_sceneTime * 6.0f + r * 1.5708f;
                float px = g_sideEnemy.pos.x + cosf(angle) * 1.3f;
                float pz = g_sideEnemy.pos.z + sinf(angle) * 1.3f;
                XMMATRIX podScale = XMMatrixScaling(0.35f, 0.35f, 0.35f);
                XMMATRIX podTrans = XMMatrixTranslation(px, g_sideEnemy.pos.y, pz);
                DrawCube(podScale * podTrans, viewProj, XMFLOAT4(0.2f, 0.8f, 1.0f, 1.0f));
            }
        }

        for (const auto& proj : g_projectiles) {
            if (!proj.active) continue;

            if (proj.isEnemyBolt) {
                XMMATRIX bScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
                XMMATRIX bRot = XMMatrixRotationRollPitchYaw(g_sceneTime * 10.0f, g_sceneTime * 15.0f, 0.0f);
                XMMATRIX bTrans = XMMatrixTranslation(proj.pos.x, proj.pos.y, proj.pos.z);
                DrawCube(bScale * bRot * bTrans, viewProj, XMFLOAT4(0.8f, 0.1f, 1.0f, 1.0f));
            }
            else {
                XMMATRIX bScale = XMMatrixScaling(0.2f, 0.2f, 1.2f);
                XMMATRIX bTrans = XMMatrixTranslation(proj.pos.x, proj.pos.y, proj.pos.z);
                DrawCube(bScale * bTrans, viewProj, XMFLOAT4(0.1f, 1.0f, 0.8f, 1.0f));
            }
        }

        if (!g_isFpsAiming) {
            bool isMoving = (GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState('S') & 0x8000) ||
                (GetAsyncKeyState('A') & 0x8000) || (GetAsyncKeyState('D') & 0x8000);

            float legAngleL = 0.0f;
            float legAngleR = 0.0f;
            float armAngleL = 0.0f;
            float armAngleR = 0.0f;
            float bodyOffsetY = 0.0f;
            float bodyScaleY = 1.0f;
            float bodyScaleXZ = 1.0f;

            if (!g_playerGrounded) {
                if (g_playerVel.y > 0.0f) {
                    bodyScaleY = 1.22f;
                    bodyScaleXZ = 0.88f;
                    armAngleL = -1.5f;
                    armAngleR = -1.5f;
                    legAngleL = 0.6f;
                    legAngleR = 0.6f;
                }
                else {
                    bodyScaleY = 0.88f;
                    bodyScaleXZ = 1.12f;
                    armAngleL = -0.7f;
                    armAngleR = -0.7f;
                    legAngleL = -0.4f;
                    legAngleR = -0.4f;
                }
            }
            else if (isMoving) {
                float runCycle = sinf(g_sceneTime * 16.0f);
                legAngleL = runCycle * 0.6f;
                legAngleR = -runCycle * 0.6f;
                armAngleL = -runCycle * 0.7f;
                armAngleR = runCycle * 0.7f;
                bodyOffsetY = fabsf(sinf(g_sceneTime * 32.0f)) * 0.08f;
            }
            else {
                bodyOffsetY = sinf(g_sceneTime * 3.5f) * 0.03f;
            }

            auto RenderRobotPartEx = [&](XMMATRIX scale, XMFLOAT3 localPos, XMFLOAT4 color, float rotX = 0.0f, XMFLOAT3 pivot = XMFLOAT3(0, 0, 0)) {
                XMMATRIX partScale = scale;
                XMMATRIX partPivotOffset = XMMatrixTranslation(localPos.x - pivot.x, localPos.y - pivot.y, localPos.z - pivot.z);
                XMMATRIX partRotX = XMMatrixRotationX(rotX);
                XMMATRIX partPivotPos = XMMatrixTranslation(pivot.x, pivot.y + bodyOffsetY, pivot.z);

                XMMATRIX partLocal = partScale * partPivotOffset * partRotX * partPivotPos;
                XMMATRIX rotYMat = XMMatrixRotationY(g_camYaw);
                XMMATRIX partWorld = XMMatrixTranslation(g_playerPos.x, g_playerPos.y, g_playerPos.z);
                XMMATRIX finalWorld = partLocal * rotYMat * partWorld;
                DrawCube(finalWorld, viewProj, color);
                };

            bool isStunned = (g_stunTimer > 0.0f);
            XMFLOAT4 mainColor = isStunned ? XMFLOAT4(1.0f, 0.9f, 0.1f, 1.0f) : XMFLOAT4(0.2f, 0.7f, 0.85f, 1.0f);

            RenderRobotPartEx(XMMatrixScaling(0.48f * bodyScaleXZ, 0.45f * bodyScaleY, 0.45f * bodyScaleXZ), XMFLOAT3(0.0f, 0.45f * bodyScaleY, 0.0f), XMFLOAT4(0.75f, 0.8f, 0.85f, 1.0f));
            RenderRobotPartEx(XMMatrixScaling(0.6f * bodyScaleXZ, 0.6f * bodyScaleY, 0.4f * bodyScaleXZ), XMFLOAT3(0.0f, -0.05f * bodyScaleY, 0.0f), mainColor);
            RenderRobotPartEx(XMMatrixScaling(0.16f, 0.5f, 0.16f), XMFLOAT3(-0.42f, -0.05f, 0.0f), XMFLOAT4(0.4f, 0.5f, 0.6f, 1.0f), armAngleL, XMFLOAT3(-0.42f, 0.15f, 0.0f));
            RenderRobotPartEx(XMMatrixScaling(0.16f, 0.5f, 0.16f), XMFLOAT3(0.42f, -0.05f, 0.0f), XMFLOAT4(0.4f, 0.5f, 0.6f, 1.0f), armAngleR, XMFLOAT3(0.42f, 0.15f, 0.0f));
            RenderRobotPartEx(XMMatrixScaling(0.2f, 0.5f, 0.2f), XMFLOAT3(-0.18f, -0.55f, 0.0f), XMFLOAT4(0.4f, 0.5f, 0.6f, 1.0f), legAngleL, XMFLOAT3(-0.18f, -0.3f, 0.0f));
            RenderRobotPartEx(XMMatrixScaling(0.2f, 0.5f, 0.2f), XMFLOAT3(0.18f, -0.55f, 0.0f), XMFLOAT4(0.4f, 0.5f, 0.6f, 1.0f), legAngleR, XMFLOAT3(0.18f, -0.3f, 0.0f));

            if (g_highJumpTimer > 0.0f) {
                RenderRobotPartEx(XMMatrixScaling(0.26f, 0.16f, 0.26f), XMFLOAT3(-0.18f, -0.82f, 0.0f), XMFLOAT4(1.0f, 0.85f, 0.0f, 1.0f), legAngleL, XMFLOAT3(-0.18f, -0.3f, 0.0f));
                RenderRobotPartEx(XMMatrixScaling(0.26f, 0.16f, 0.26f), XMFLOAT3(0.18f, -0.82f, 0.0f), XMFLOAT4(1.0f, 0.85f, 0.0f, 1.0f), legAngleR, XMFLOAT3(0.18f, -0.3f, 0.0f));

                float auraY = -0.3f + sinf(g_sceneTime * 8.0f) * 0.2f;
                RenderRobotPartEx(XMMatrixScaling(0.75f, 0.05f, 0.75f), XMFLOAT3(0.0f, auraY, 0.0f), XMFLOAT4(1.0f, 0.9f, 0.2f, 0.8f));
            }

            if (g_flyTimer > 0.0f) {
                RenderRobotPartEx(XMMatrixScaling(0.45f, 0.55f * bodyScaleY, 0.22f), XMFLOAT3(0.0f, 0.05f * bodyScaleY, -0.28f), XMFLOAT4(0.1f, 0.85f, 0.95f, 1.0f));
                RenderRobotPartEx(XMMatrixScaling(0.12f, 0.15f, 0.12f), XMFLOAT3(-0.15f, -0.28f * bodyScaleY, -0.28f), XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
                RenderRobotPartEx(XMMatrixScaling(0.12f, 0.15f, 0.12f), XMFLOAT3(0.15f, -0.28f * bodyScaleY, -0.28f), XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));

                float flameScale = 0.3f + sinf(g_sceneTime * 35.0f) * 0.1f;
                RenderRobotPartEx(XMMatrixScaling(0.12f, flameScale, 0.12f), XMFLOAT3(-0.15f, -0.32f * bodyScaleY - flameScale * 0.5f, -0.28f), XMFLOAT4(1.0f, 0.4f, 0.0f, 1.0f));
                RenderRobotPartEx(XMMatrixScaling(0.12f, flameScale, 0.12f), XMFLOAT3(0.15f, -0.32f * bodyScaleY - flameScale * 0.5f, -0.28f), XMFLOAT4(1.0f, 0.4f, 0.0f, 1.0f));
            }

            if (g_ammoCount > 0) {
                RenderRobotPartEx(XMMatrixScaling(0.12f, 0.14f, 0.6f), XMFLOAT3(0.42f, 0.05f, 0.35f), XMFLOAT4(0.2f, 0.2f, 0.25f, 1.0f), armAngleR, XMFLOAT3(0.42f, 0.15f, 0.0f));
                RenderRobotPartEx(XMMatrixScaling(0.18f, 0.18f, 0.18f), XMFLOAT3(0.42f, 0.05f, 0.65f), XMFLOAT4(0.8f, 0.1f, 0.9f, 1.0f), armAngleR, XMFLOAT3(0.42f, 0.15f, 0.0f));
            }

            if (isStunned) {
                for (int k = 0; k < 4; ++k) {
                    float sx = sinf(g_sceneTime * 40.0f + k * 1.57f) * 0.6f;
                    float sy = cosf(g_sceneTime * 30.0f + k * 1.57f) * 0.6f;
                    RenderRobotPartEx(XMMatrixScaling(0.1f, 0.1f, 0.1f), XMFLOAT3(sx, sy, 0.0f), XMFLOAT4(1.0f, 0.9f, 0.0f, 1.0f));
                }
            }
        }
    }

    DrawUIQuad();
    g_pSwapChain->Present(1, 0);
}

void Release() {
    if (g_hMidi) { midiOutReset(g_hMidi); midiOutClose(g_hMidi); }
    if (g_hFontTitle) DeleteObject(g_hFontTitle);
    if (g_hFontBig) DeleteObject(g_hFontBig);
    if (g_hFontMed) DeleteObject(g_hFontMed);
    if (g_hFontSub) DeleteObject(g_hFontSub);
    if (g_hUIBitmap) DeleteObject(g_hUIBitmap);
    if (g_hMemDC) DeleteDC(g_hMemDC);

    SafeRelease(&g_pUIIndexBuffer); SafeRelease(&g_pUIVertexBuffer); SafeRelease(&g_pUIBlendState); SafeRelease(&g_pUISRV); SafeRelease(&g_pUITexture);
    SafeRelease(&g_pSamplerState); SafeRelease(&g_pTextureSRV); SafeRelease(&g_pConstantBuffer); SafeRelease(&g_pIndexBuffer); SafeRelease(&g_pVertexBuffer);
    SafeRelease(&g_pInputLayout); SafeRelease(&g_pVS); SafeRelease(&g_pPS); SafeRelease(&g_pDSV); SafeRelease(&g_pDepthBuffer); SafeRelease(&g_pRTV);
    SafeRelease(&g_pSwapChain); SafeRelease(&g_pContext); SafeRelease(&g_pDevice);
}

// ==========================================
void InputKey(WPARAM wParam) {
    ProcessInput(wParam); // 로봇 점프 내부의 키보드 처리 함수 호출
}
void InputMouseClick(int mx, int my) {
    int mappedX = (mx * 1024) / 1280;
    int mappedY = (my * 768) / 720;
    HandleMouseClick(mappedX, mappedY); // 로봇 점프 내부의 마우스 클릭 함수 호출
    if (g_gameState == STATE_TITLE && mappedX >= 362 && mappedX <= 662 && mappedY >= 570 && mappedY <= 630) {
        gameovercheck = true;
    }
}
void InputMouseMove(int mx, int my) {
    // 로봇 점프는 클릭만 사용하므로 비워둬도 됨
}
bool IsGameOver() {

    return gameovercheck; // 실제 게임오버 변수 연결 필요
}
bool IsForceEnd() { return isForceEndRelay; }
int GetScore() {
    return (int)g_jumpScore; // 혹은 현재 survivalTime 반환
}
}