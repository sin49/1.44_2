#include "FruitManager.h"
#include "../DX2DClasses/Colliders.h"

#include <cstdlib>
#include <ctime>

using namespace DX2DClasses;

CFruitManager::CFruitManager()
    : m_hWnd(nullptr)
    , m_pDX2DFramework(nullptr)
    , m_pCherryImage(nullptr)
    , m_pAppleImage(nullptr)
    , m_pBombImage(nullptr)
	, m_pDebugBrush(nullptr)
    , m_fGameTime(0.0f)
    , m_fSpawnTimer(0.0f)
{
}

CFruitManager::~CFruitManager()
{
    Release();
}

void CFruitManager::Initialize(
    HWND hWnd,
    CDriect2DFramwork* pDX2DFramework)
{
    m_hWnd = hWnd;
    m_pDX2DFramework = pDX2DFramework;

    m_fGameTime = 0.0f;
    m_fSpawnTimer = 0.0f;

    // 랜덤 시드 초기화
    srand(static_cast<unsigned int>(time(nullptr)));

    // Cherry 이미지 생성
    m_pCherryImage = new CImage(
        m_pDX2DFramework->GetD2DRenderTarget(),
        m_pDX2DFramework->GetImagingFactory(),
        1
    );

    // Cherry 이미지 로드
    m_pCherryImage->ManualLoadImage(
        m_hWnd,
        L"Images\\Items\\cherry.png"
    );

    m_pAppleImage = new CImage(
        m_pDX2DFramework->GetD2DRenderTarget(),
        m_pDX2DFramework->GetImagingFactory(),
        1
    );

    m_pAppleImage->ManualLoadImage(
        m_hWnd,
        L"Images\\Items\\apple.png"
    );

    // -------------------------
    // Bomb
    // -------------------------
    m_pBombImage = new CImage(
        m_pDX2DFramework->GetD2DRenderTarget(),
        m_pDX2DFramework->GetImagingFactory(),
        1
    );

    m_pBombImage->ManualLoadImage(
        m_hWnd,
        L"Images\\Items\\bomb.png"
    );
}

void CFruitManager::Update()
{
    // 현재 프레임을 약 1/60초로 계산
    const float deltaTime = 1.0f / 60.0f;

    m_fGameTime += deltaTime;
    m_fSpawnTimer += deltaTime;

    // 게임 시간이 60초가 넘으면 더 이상 생성하지 않음
    if (m_fGameTime >= 60.0f)
    {
        // 이미 생성된 과일은 계속 떨어지게 한다.
        for (CFruit* pFruit : m_vFruits)
        {
            if (pFruit)
                pFruit->Update();
        }

        return;
    }

    float spawnInterval = 0.0f;

    // -------------------------
    // 0 ~ 10초
    // -------------------------
    if (m_fGameTime < 10.0f)
    {
        spawnInterval = 2.0f;
    }

    // -------------------------
    // 10 ~ 30초
    // -------------------------
    else if (m_fGameTime < 20.0f)
    {
        spawnInterval = 1.0f;
    }

    // -------------------------
    // 30 ~ 60초
    // -------------------------
	else if (m_fGameTime < 30.0f)
    {
        spawnInterval = 0.5f;
    }
    else
    {
        spawnInterval = 0.25f;
    }

    // 생성 시간이 되었다면 Cherry 생성
    if (m_fSpawnTimer >= spawnInterval)
    {
        m_fSpawnTimer = 0.0f;

        SpawnFruit();
    }

    // 모든 과일 업데이트
    for (CFruit* pFruit : m_vFruits)
    {
        if (pFruit)
            pFruit->Update();
    }
}

void CFruitManager::Draw()
{
    for (CFruit* pFruit : m_vFruits)
    {
        if (!pFruit)
            continue;

        // 과일 그리기
        pFruit->Draw();

        //// Collider 그리기
        //if (pFruit->GetCollider())
        //{
        //    pFruit->GetCollider()->DrawOutline(
        //        m_pDebugBrush,
        //        2.0f
        //    );
        //}
    }
}

void CFruitManager::Release()
{
    // 생성된 과일 제거
    for (CFruit* pFruit : m_vFruits)
    {
        if (pFruit)
        {
            delete pFruit;
        }
    }

    m_vFruits.clear();

    // Cherry 이미지 제거
    if (m_pCherryImage)
    {
        delete m_pCherryImage;
        m_pCherryImage = nullptr;
    }

    if (m_pAppleImage)
    {
        delete m_pAppleImage;
        m_pAppleImage = nullptr;
    }

    if (m_pBombImage)
    {
        delete m_pBombImage;
        m_pBombImage = nullptr;
    }

    m_hWnd = nullptr;
    m_pDX2DFramework = nullptr;
}

void CFruitManager::ClearFruits()
{
    for (CFruit* pFruit : m_vFruits)
    {
        if (pFruit)
            delete pFruit;
    }

    m_vFruits.clear();

    m_fGameTime = 0.0f;
    m_fSpawnTimer = 0.0f;
}

void CFruitManager::SpawnCherry()
{
    CFruit* pCherry = new CFruit();

    // -------------------------
    // X 위치 랜덤 설정
    // -------------------------

    // 예: 게임 화면의 좌우 범위를 50 ~ 550으로 설정
    float x = static_cast<float>(rand() % 500 + 50);

    // 화면 위쪽에서 생성
    float y = 50.0f;

    pCherry->GetTransform().SetTransrate(x, y);

    // -------------------------
    // 시간에 따른 낙하 속도
    // -------------------------

    if (m_fGameTime < 10.0f)
    {
        pCherry->SetFallSpeed(2.0f);
    }
    else if (m_fGameTime < 30.0f)
    {
        pCherry->SetFallSpeed(3.0f);
    }
    else if (m_fGameTime < 45.0f)
    {
        pCherry->SetFallSpeed(5.0f);
    }
    else
    {
        pCherry->SetFallSpeed(7.0f);
    }

    // Cherry 이미지 설정
    pCherry->Initialize(
        m_pCherryImage,
        false,
        true
    );

    CCircleCollider* pCollider =
        new CCircleCollider(
            pCherry->GetTransformPtr()
        );

    // Collider의 위치와 크기 설정
    pCollider->InitCollider(
        pCherry->GetTransformPtr(),
        SVector2(11.0f, 11.0f),
        m_pCherryImage->GetImageSize(),
        0.3f
    );

    pCherry->SetCollider(pCollider);

    // 벡터에 추가
    m_vFruits.push_back(pCherry);
}

CFruit* CFruitManager::RemoveStackedFruits(int count)
{
    for (int i = 0; i < count; ++i)
    {
        CFruit* pTopFruit = nullptr;

        // 현재 가장 위에 있는 과일 찾기
        for (CFruit* pFruit : m_vFruits)
        {
            if (pFruit == nullptr)
                continue;

            if (!pFruit->IsStacked())
                continue;

            if (pTopFruit == nullptr)
            {
                pTopFruit = pFruit;
                continue;
            }

            float currentY =
                pFruit->GetTransform().GetTransrate().y;

            float topY =
                pTopFruit->GetTransform().GetTransrate().y;

            if (currentY < topY)
            {
                pTopFruit = pFruit;
            }
        }

        // 삭제할 과일이 없으면 종료
        if (pTopFruit == nullptr)
            break;

        // 벡터에서 찾아서 삭제
        for (auto it = m_vFruits.begin();
            it != m_vFruits.end();
            ++it)
        {
            if (*it == pTopFruit)
            {
                delete* it;
                m_vFruits.erase(it);
                break;
            }
        }
    }

    // 삭제 후 가장 위에 있는 과일 찾기
    CFruit* pTopFruit = nullptr;

    for (CFruit* pFruit : m_vFruits)
    {
        if (pFruit == nullptr)
            continue;

        if (!pFruit->IsStacked())
            continue;

        if (pTopFruit == nullptr)
        {
            pTopFruit = pFruit;
            continue;
        }

        float currentY =
            pFruit->GetTransform().GetTransrate().y;

        float topY =
            pTopFruit->GetTransform().GetTransrate().y;

        if (currentY < topY)
        {
            pTopFruit = pFruit;
        }
    }

    return pTopFruit;
}

SVector2 CFruitManager::GetCherryImageSize()
{
    if (m_pCherryImage == nullptr)
        return SVector2(0.0f, 0.0f);

    return m_pCherryImage->GetImageSize();
}

void CFruitManager::CreateCollider(CFruit* pFruit)
{
    if (pFruit == nullptr)
        return;

    if (pFruit->GetCollider() != nullptr)
        return;

    CCircleCollider* pCollider =
        new CCircleCollider(
            pFruit->GetTransformPtr()
        );

    pCollider->InitCollider(
        pFruit->GetTransformPtr(),
        SVector2(11.0f, 11.0f),
        m_pCherryImage->GetImageSize(),
        0.3f
    );

    pFruit->SetCollider(pCollider);
}

void CFruitManager::SpawnFruit()
{
    CFruit* pFruit = new CFruit();

    // -------------------------
    // 과일 종류 결정
    // -------------------------

    int randomValue = rand() % 100;

    EFruitType fruitType;
    CImage* pImage = nullptr;

    if (randomValue < 40)
    {
        // 0 ~ 39 : Cherry 40%
        fruitType = EFruitType::CHERRY;
        pImage = m_pCherryImage;
    }
    else if (randomValue < 80)
    {
        // 40 ~ 79 : Apple 40%
        fruitType = EFruitType::APPLE;
        pImage = m_pAppleImage;
    }
    else
    {
        // 80 ~ 99 : Bomb 20%
        fruitType = EFruitType::BOMB;
        pImage = m_pBombImage;
    }

    pFruit->SetFruitType(fruitType);

    // -------------------------
    // X 위치 랜덤
    // -------------------------

    float x =
        static_cast<float>(rand() % 500 + 50);

    float y = 50.0f;

    pFruit->GetTransform().SetTransrate(x, y);

    // -------------------------
    // 시간에 따른 낙하 속도
    // -------------------------

    if (m_fGameTime < 10.0f)
    {
        pFruit->SetFallSpeed(2.0f);
    }
    else if (m_fGameTime < 20.0f)
    {
        pFruit->SetFallSpeed(3.0f);
    }
    else if (m_fGameTime < 30.0f)
    {
        pFruit->SetFallSpeed(5.0f);
    }
    else if (m_fGameTime < 45.0f)
    {   
        pFruit->SetFallSpeed(7.5f);
    }
    else
    {
        pFruit->SetFallSpeed(10.0f);
    }

    // -------------------------
    // 이미지 설정
    // -------------------------

    pFruit->Initialize(
        pImage,
        false,
        true
    );

    // -------------------------
    // Collider 생성
    // -------------------------

    CCircleCollider* pCollider =
        new CCircleCollider(
            pFruit->GetTransformPtr()
        );

    SVector2 colliderSize;

    switch (fruitType)
    {
    case EFruitType::CHERRY:
        colliderSize = SVector2(11.0f, 11.0f);
        break;

    case EFruitType::APPLE:
        colliderSize = SVector2(16.0f, 16.0f);
        break;

    case EFruitType::BOMB:
        colliderSize = SVector2(16.0f, 16.0f);
        break;
    }

    pCollider->InitCollider(
        pFruit->GetTransformPtr(),
        colliderSize,
        pImage->GetImageSize(),
        0.3f
    );

    pFruit->SetCollider(pCollider);

    // -------------------------
    // 리스트에 추가
    // -------------------------

    m_vFruits.push_back(pFruit);
}

void CFruitManager::RemoveBomb(CFruit* pFruit)
{
    for (auto it = m_vFruits.begin();
        it != m_vFruits.end();
        ++it)
    {
        if (*it == pFruit)
        {
            delete* it;
            m_vFruits.erase(it);
            return;
        }
    }
}
        
    
