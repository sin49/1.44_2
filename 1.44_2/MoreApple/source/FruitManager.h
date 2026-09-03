#pragma once

#include <vector>

#include "Fruit.h"

#include "../DX2DClasses/Driect2DFramework.h"
#include "../DX2DClasses/Image.h"
#include "../DX2DClasses/ColorBrush.h"
#include "../DX2DClasses/Vector2.h"

class CFruitManager
{
private:
    CFruit* m_pTopFruit;
    std::vector<CFruit*> m_vFruits;

    HWND m_hWnd;
    DX2DClasses::CDriect2DFramwork* m_pDX2DFramework;
	DX2DClasses::CColorBrush* m_pDebugBrush;

	DX2DClasses::CImage* m_pCherryImage;
    DX2DClasses::CImage* m_pAppleImage;
    DX2DClasses::CImage* m_pBombImage;

    float m_fGameTime;
    float m_fSpawnTimer;

public:
    CFruitManager();
    ~CFruitManager();

public:
    void Initialize(
        HWND hWnd,
        DX2DClasses::CDriect2DFramwork* pDX2DFramework
    );

    void ClearFruits();

    void Update();
    void Draw();
    void Release();

    CFruit* RemoveStackedFruits(int count);
    DX2DClasses::SVector2 GetCherryImageSize();

	std::vector<CFruit*>& GetFruits() { return m_vFruits; }
    void CreateCollider(CFruit* pFruit);

    void RemoveBomb(CFruit* pFruit);

private:
    void SpawnCherry();
    void SpawnFruit();
};
