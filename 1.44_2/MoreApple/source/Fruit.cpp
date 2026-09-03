#include "Fruit.h"

#include "../DX2DClasses/Vector2.h"

using namespace DX2DClasses;

CFruit::CFruit()
    : m_fFallSpeed(1.0f)
    , m_isStacked(false)
	, m_vStackOffset(0.0f, 0.0f)
    , m_eFruitType(EFruitType::CHERRY)
{
}

CFruit::~CFruit()
{
    Release();
}

void CFruit::SetFallSpeed(float speed)
{
    m_fFallSpeed = speed;
}

void CFruit::SetStacked(bool stacked)
{
    m_isStacked = stacked;
}

bool CFruit::IsStacked() const
{
    return m_isStacked;
}

void CFruit::SetStackOffset(const SVector2& offset)
{
    m_vStackOffset = offset;
}

SVector2 CFruit::GetStackOffset() const
{
    return m_vStackOffset;
}

void CFruit::Update()
{
    if (!m_isStacked)
    {
        CTransform& transform = GetTransform();

        SVector2 pos = transform.GetTransrate();

        pos.y += m_fFallSpeed;

        transform.SetTransrate(pos);
    }

    CGameObject::Update();
}

void CFruit::Release()
{
    RemoveCollider();

    CGameObject::Release();
}

void CFruit::RemoveCollider()
{
    if (GetCollider())
    {
        delete GetCollider();
        SetCollider(nullptr);
    }
}

void CFruit::SetFruitType(EFruitType type)
{
    m_eFruitType = type;
}

EFruitType CFruit::GetFruitType() const
{
    return m_eFruitType;
}
