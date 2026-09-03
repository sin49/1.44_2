#include "Basket.h"

#include "../DX2DClasses/Colliders.h"
#include "../DX2DClasses/ColorBrush.h"
#include "../DX2DClasses/Driect2DFramework.h"

using namespace DX2DClasses;

CBasket::CBasket()
    : m_vOffset(0.0f, 0.0f)
{
}

CBasket::~CBasket()
{
    Release();
}

void CBasket::SetOffset(const SVector2& offset)
{
    m_vOffset = offset;
}

void CBasket::UpdatePosition(
    const SVector2& playerPos,
    bool isLeft)
{
    SVector2 basketPos;

    float offsetX = 0.0f;

    if (isLeft)
        offsetX = -30.0f;

    basketPos.x = playerPos.x + offsetX;
    basketPos.y = playerPos.y + m_vOffset.y;

    GetTransform().SetTransrate(basketPos);
}

void CBasket::Update()
{
    CGameObject::Update();
}

void CBasket::Release()
{
    RemoveCollider();

    CGameObject::Release();
}

void CBasket::Draw()
{
    // Basket 이미지 그리기
    CGameObject::Draw();
}

void CBasket::InitializeCollider()
{
    CRectCollider* pCollider =
        new CRectCollider();

    pCollider->InitCollider(
        GetTransformPtr(),
        SVector2(0.0f, 0.0f),
        SVector2(32.0f, 32.0f),
        1.0f
    );

    SetCollider(pCollider);
}

void CBasket::RemoveCollider()
{
    if (GetCollider())
    {
        delete GetCollider();
        SetCollider(nullptr);
    }
}
