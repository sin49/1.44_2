#pragma once

#include "../DX2DClasses/GameObject.h"
#include "../DX2DClasses/Vector2.h"

enum class EFruitType
{
    CHERRY,
    APPLE,
    BOMB
};

class CFruit : public DX2DClasses::CGameObject
{
private:
    EFruitType m_eFruitType;
    bool m_isStacked;
    float m_fFallSpeed;

    DX2DClasses::SVector2 m_vStackOffset;

public:
    CFruit();
    virtual ~CFruit();

public:
    void SetStackOffset(const DX2DClasses::SVector2& offset);
    DX2DClasses::SVector2 GetStackOffset() const;
    void SetStacked(bool stacked);
    bool IsStacked() const;

    void SetFallSpeed(float speed);

    void RemoveCollider();

    void SetFruitType(EFruitType type);
    EFruitType GetFruitType() const;

    virtual void Update() override;
	virtual void Release() override;
};
